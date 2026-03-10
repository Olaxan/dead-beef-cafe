#include "net_mgr.h"
#include "uuid.h"
#include "os.h"
#include "host.h"
#include "nic.h"
#include "filesystem.h"

#include "proto/ip_packet.pb.h"

#include <print>
#include <memory>
#include <ranges>
#include <algorithm>

NetManager::NetManager(OS* owner) : os_(*owner), nic_(owner->get_owner().get_device<NIC>())
{ }

NetManager::~NetManager() = default;

std::expected<uint64_t, std::runtime_error> NetManager::create_socket()
{

	FileSystem* fs = os_.get_filesystem();

	FilePath path{std::format("/run/{}.sock", ++socket_index_)};
	FileSystem::CreateFileParams params{ .recurse = true };

	if (auto [fid, ptr, err] = fs->create_file<SocketFile>(path, params); err == FileSystemError::Success)
	{
		sockets_[fid] = std::static_pointer_cast<SocketFile>(ptr);
		return fid;
	}
	else 
	{
		return std::unexpected{std::runtime_error{fs->get_fserror_name(err)}};
	}
}

int32_t NetManager::bind_socket(SocketDescriptor sock, AddressPair addr)
{
	if (bindings_.contains(addr))
		return 1;

	bindings_[addr] = sock;

	if (SocketFile* file = find_socket(sock))
		file->local_endpoint = addr;

	return 0;
}

int32_t NetManager::bind_socket(SocketDescriptor sock, Address6 addr, int32_t port)
{
	return bind_socket(sock, {addr, port});
}

Task<int32_t> NetManager::async_connect_socket(SocketDescriptor sock, AddressPair dest)
{
	if (auto&& it = sockets_.find(sock); it != sockets_.end())
	{
		const SocketFile* file = it->second.get();
		const AddressPair& src = file->local_endpoint;

		ip::TcpPacket tcp;
		tcp.set_src_port(src.port);
		tcp.set_dest_port(dest.port);
		tcp.set_type(ip::TcpType::Syn);

		std::string tcp_data;
		if (!tcp.SerializeToString(&tcp_data))
			co_return -1;

		ip::IpPackage ip;
		ip.set_src_ip(src.addr.raw);
		ip.set_dest_ip(dest.addr.raw);
		ip.set_protocol(ip::Protocol::TCP);
		ip.set_payload(tcp_data);
		
		send(std::move(ip));

		std::println("Awaiting ACK from {}:{}...", dest.addr, dest.port);
		
		while (true)
		{
			std::string reply_bytes = co_await async_read_socket(sock);
			ip::TcpPacket reply;
			if (reply.ParseFromString(reply_bytes))
			{
				if (reply.type() == ip::TcpType::Ack)
				{
					std::println("Received ACK from {}.", dest.addr);
					co_return 0;
				}
			}
		}
	}

	co_return -1;
}

Task<int32_t> NetManager::async_connect_socket(SocketDescriptor sock, Address6 addr, int32_t port)
{
	return async_connect_socket(sock, {addr, port});
}

Task<std::string> NetManager::async_read_socket(SocketDescriptor sock)
{
	if (auto&& it = sockets_.find(sock); it != sockets_.end())
	{
		std::shared_ptr<SocketFile> file = it->second;
		ip::IpPackage pak = co_await file->rx_queue.async_pop();
		co_return pak.payload();
	}

	co_return {};
}

Task<ip::IpPackage> NetManager::async_read_socket_raw(SocketDescriptor sock)
{
	if (auto&& it = sockets_.find(sock); it != sockets_.end())
	{
		std::shared_ptr<SocketFile> file = it->second;
		co_return (co_await file->rx_queue.async_pop());
	}

	co_return {};
}

Task<size_t> NetManager::async_write_socket(SocketDescriptor sock, std::string bytes)
{
	if (auto&& it = sockets_.find(sock); it != sockets_.end())
	{
		it->second->write(bytes);
		process_sockets();
		co_return bytes.size();
	}
	co_return 0;
}

int32_t NetManager::listen(SocketDescriptor sock)
{
	if (SocketFile* file = find_socket(sock))
	{
		return 0;
	}
	return 1;
}

Task<int32_t> NetManager::async_accept_socket(SocketDescriptor sock)
{
	while (true)
	{
		ip::IpPackage raw = co_await async_read_socket_raw(sock);

		auto exp_src_addr = Address6::from_bytes(raw.src_ip());
		if (!exp_src_addr)
			continue;

		ip::TcpPacket syn;
		if (!syn.ParseFromString(raw.payload()))
			continue;

		if (syn.type() == ip::TcpType::Syn)
		{
			// auto exp_fd = create_socket();
			// if (!exp_fd)
			// 	co_return -1;

			// if (SocketFile* file = find_socket(sock))
			// {
			// 	file->other_endpoint = { *exp_src_addr, syn.dest_port() };
			// }
			// else co_return -1;

			ip::IpPackage reply;
			reply.set_dest_ip(raw.src_ip());
			reply.set_src_ip(raw.dest_ip());
			reply.set_protocol(ip::Protocol::TCP);

			ip::TcpPacket ack;
			ack.set_dest_port(syn.src_port());
			ack.set_src_port(syn.dest_port());
			ack.set_type(ip::TcpType::Ack);

			std::string ack_data;
			if (ack.SerializeToString(&ack_data))
			{
				reply.set_payload(ack_data);
				send(std::move(reply));
				co_return 0;
			}
		}
	}

	co_return -1;
}

void NetManager::add_socket_filter(SocketFilter&& filter)
{
	socket_filters_.push_back(std::move(filter));
}

void NetManager::remove_socket_filter(void* listener)
{
	std::erase_if(socket_filters_, [listener](const SocketFilter& filter)
	{
		return (filter.listener == listener);
	});
}

void NetManager::send(ip::IpPackage&& package)
{
	assert(nic_);
	nic_->get_tx_queue().push(std::move(package));
}

void NetManager::send(ip::IpPackage&& package, UUID mac)
{
	assert(nic_);
	nic_->transfer(mac, std::move(package));
}

void NetManager::receive(ip::IpPackage&& package)
{
	const std::string& dest_ip_bytes = package.dest_ip();
	if (auto exp_dest_ip = Address6::from_bytes(dest_ip_bytes.data()))
	{
		receive(std::move(package), *exp_dest_ip);
	}
}

void NetManager::receive(ip::IpPackage&& package, const Address6& addr)
{
	const std::string& payload = package.payload();

	switch (package.protocol())
	{
		case ip::Protocol::ICMP:
		{
			ip::IcmpPacket icmp;
			if (icmp.ParseFromString(payload))
			{
				handle_packet(std::move(icmp), std::move(package), addr);
			}
			break;
		}
		case ip::Protocol::TCP:
		{
			ip::TcpPacket tcp;
			if (tcp.ParseFromString(payload))
			{
				handle_packet(std::move(tcp), std::move(package), addr);
			}
			break;
		}
		case ip::Protocol::UDP:
		{
			ip::UdpPacket udp;
			if (udp.ParseFromString(payload))
			{
				handle_packet(std::move(udp), std::move(package), addr);
			}
			break;
		}
		default: break;
	}
}

void NetManager::handle_packet(ip::IcmpPacket&& packet, ip::IpPackage&& outer, const Address6& addr)
{
	switch (packet.type())
	{
		case ip::IcmpType::EchoRequest:
		{
			if (auto exp_src = Address6::from_bytes(outer.src_ip()); exp_src.has_value())
			{
				std::println("Received echo request from {}.", *exp_src);
			}

			ip::IcmpPacket reply;
			reply.set_type(ip::IcmpType::EchoReply);
			std::string ret_str;
			if (reply.SerializeToString(&ret_str))
			{
				ip::IpPackage pak;
				pak.set_dest_ip(outer.src_ip());
				pak.set_src_ip(get_primary_ip().raw);
				pak.set_protocol(ip::Protocol::ICMP);
				pak.set_payload(ret_str);
				send(std::move(pak));
			}
			break;
		}
		case ip::IcmpType::EchoReply:
		{
			if (auto exp_src = Address6::from_bytes(outer.src_ip()); exp_src.has_value())
			{
				std::println("Received echo reply from {}.", *exp_src);
			}
			break;
		}
		default: break;
	}
}

void NetManager::handle_packet(ip::TcpPacket&& packet, ip::IpPackage&& outer, const Address6& addr)
{
	AddressPair pair = { addr, packet.dest_port() };
	if (SocketFile* sock = find_socket(pair))
	{
		sock->rx_queue.push(std::move(outer));
		//sock->notify_write();
	}
	/* In the future, we should return an ACK here. */
}

void NetManager::handle_packet(ip::UdpPacket&& packet, ip::IpPackage&& outer, const Address6& addr)
{
	AddressPair pair = { addr, packet.dest_port() };
	if (SocketFile* sock = find_socket(pair))
	{
		sock->rx_queue.push(std::move(outer));
		//sock->notify_write();
	}
}

NetMessageAwaiter NetManager::async_read_rx()
{
	assert(nic_);
	return nic_->get_rx_queue().async_pop();
}

NetMessageAwaiter NetManager::async_read_tx()
{
	assert(nic_);
	return nic_->get_tx_queue().async_pop();
}

Address6 NetManager::get_primary_ip() const
{
	assert(nic_);
	return nic_->get_ip();
}

bool NetManager::socket_is_open(SocketDescriptor fd) const
{
	return (sockets_.contains(fd));
}

SocketFile* NetManager::find_socket(SocketDescriptor sock_fd)
{
	if (auto it_sock = sockets_.find(sock_fd); it_sock != sockets_.end())
		return it_sock->second.get();

	return nullptr;
}

SocketFile* NetManager::find_socket(const AddressPair& tuple)
{
	if (auto it_fd = bindings_.find(tuple); it_fd != bindings_.end())
		return find_socket(it_fd->second);

	return nullptr;
}

void NetManager::broadcast_socket_rx(SocketDescriptor fd, const std::string& payload)
{
	for (auto&& filter : socket_filters_)
	{
		if (filter.fd == fd)
		{
			filter.fn(payload);
		}
	}
}

void NetManager::process_sockets()
{
	for (auto& [fd, sock] : sockets_)
	{
		if (std::optional<std::string> data = sock->read_tx(); data.has_value())
		{
			const AddressPair& src = sock->local_endpoint;
			const AddressPair& dest = sock->other_endpoint;

			ip::TcpPacket tcp{};
			tcp.set_dest_port(dest.port);
			tcp.set_src_port(src.port);
			tcp.set_payload(*data);

			std::string tcp_data{};
			if (!tcp.SerializeToString(&tcp_data))
				continue;

			ip::IpPackage pak{};
			pak.set_dest_ip(dest.addr.raw);
			pak.set_src_ip(src.addr.raw);
			pak.set_payload(tcp_data);
			pak.set_protocol(ip::Protocol::TCP);

			std::println("Dispatch {}:{} --> {}:{}", src.addr, src.port, dest.addr, dest.port);

			if (dest.addr == src.addr)
			{
				receive(std::move(pak));
			}
			else
			{
				send(std::move(pak));
			}
		}
	}
}

LinkUpdateAwaiter NetManager::async_await_link()
{
	return LinkUpdateAwaiter{nic_};
}

void NetManager::arp_request()
{
	nic_->broadcast([this](UUID mac, NIC* nic)
	{
		Address6 addr = nic->get_ip();
		arp_cache_[addr] = mac;
		std::println("- ARP entry: {} -> {}", addr, mac);
	});
}

void NetManager::arp_request(UUID mac)
{
	nic_->unicast(mac, [this](UUID mac, NIC* nic)
	{
		Address6 addr = nic->get_ip();
		arp_cache_[addr] = mac;
		std::println("- ARP entry: {} -> {}", addr, mac);
	});
}

std::optional<UUID> NetManager::arp_lookup(Address6 addr)
{
	if (auto it = arp_cache_.find(addr); it != arp_cache_.end())
	{
		return it->second;
	}
	return std::nullopt;
}
