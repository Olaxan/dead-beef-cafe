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
#include <tuple>

#include <iso646.h>

NetManager::NetManager(OS* owner) 
: os_(owner), nic_(owner->get_owner().get_device<NIC>()) { }

NetManager::~NetManager() = default;

std::expected<OpenSocketPair, std::error_condition> NetManager::create_socket()
{
	OpenSocketHandle h = get_handle();
	auto [it, success] = sockets_.try_emplace(h);

	if (success)
	{
		return std::make_pair(h, &it->second);
	}
	else
	{
		return_handle(h);
		return std::unexpected{std::error_condition{EIO, std::generic_category()}};
	}
}

std::error_condition NetManager::close_socket(OpenSocketHandle h)
{
	std::println("Closing socket {}...", h);
	if (auto it = sockets_.find(h); it != sockets_.end())
	{
		OpenSocketEntry* entry = &it->second;
		entry->rx_queue.broadcast_clear({});
		entry->tx_queue.broadcast_clear({});
		return_handle(h);
		sockets_.erase(it);
		return {};
	} 
	else return std::error_condition{EBADF, std::generic_category()};
}

Task<std::error_condition> NetManager::async_close_socket(OpenSocketHandle h)
{
	if (auto it = sockets_.find(h); it != sockets_.end())
	{
		OpenSocketEntry* entry = &it->second;
		
		if (auto opt_reply = make_tcp_reply_ip(h, ip::TcpType::Fin, {}))
		{
			send(std::move(*opt_reply));
		}

		if (auto opt_query = make_tcp_query_ip(h, ip::TcpType::Fin, {}))
		{
			receive(std::move(*opt_query));
		}

		while (socket_has_data(h))
		{
			co_await os_->wait(0);
		}

		co_return close_socket(h);
	}
	else co_return std::error_condition{EBADF, std::generic_category()};
}

std::error_condition NetManager::bind_socket(OpenSocketHandle sock, AddressPair addr)
{
	if (bindings_.contains(addr))
		return std::error_condition{EADDRINUSE, std::generic_category()};

	bindings_[addr] = sock;

	if (OpenSocketEntry* file = find_socket(sock))
		file->local_endpoint = addr;

	return {};
}

std::error_condition NetManager::bind_socket(OpenSocketHandle sock, Address6 addr, int32_t port)
{
	return bind_socket(sock, {addr, port});
}

Task<std::error_condition> NetManager::async_connect_socket(OpenSocketHandle sock, AddressPair dest)
{
	if (OpenSocketEntry* file = find_socket(sock))
	{
		const AddressPair& src = file->local_endpoint;

		ip::TcpPacket tcp;
		tcp.set_src_port(src.port);
		tcp.set_dest_port(dest.port);
		tcp.set_type(ip::TcpType::Syn);

		std::string tcp_data;
		if (!tcp.SerializeToString(&tcp_data))
			co_return std::error_condition{EIO, std::generic_category()};

		ip::IpPackage ip;
		ip.set_src_ip(src.addr.raw);
		ip.set_dest_ip(dest.addr.raw);
		ip.set_protocol(ip::Protocol::TCP);
		ip.set_payload(tcp_data);
		
		send(std::move(ip));

		std::println("Awaiting ACK from {}:{}...", dest.addr, dest.port);
		
		while (true)
		{
			auto exp_reply = co_await async_read_socket_tcp(sock);

			if (not exp_reply)
				break;

			if (exp_reply->type() == ip::TcpType::Fin)
				break;

			if (exp_reply->type() == ip::TcpType::Ack)
			{
				std::println("Received ACK from {}.", dest.addr);
				file->remote_endpoint = dest;
				create_session(sock);
				co_return {};
			}
		}
	}

	co_return std::error_condition{EIO, std::generic_category()};
}

Task<std::error_condition> NetManager::async_connect_socket(OpenSocketHandle sock, Address6 addr, int32_t port)
{
	return async_connect_socket(sock, {addr, port});
}

Task<NetReadResult> NetManager::async_read_socket(OpenSocketHandle sock)
{
	if (auto exp_pak = co_await async_read_socket_tcp(sock))
	{
		co_return exp_pak->payload();
	}

	co_return std::unexpected{std::error_condition{EIO, std::generic_category()}};
}

Task<NetReadResultTcp> NetManager::async_read_socket_tcp(OpenSocketHandle sock)
{
	if (auto exp_pak = co_await async_read_socket_raw(sock))
	{
		ip::TcpPacket tcp;
		if (tcp.ParseFromString(exp_pak->payload()))
			co_return tcp;
	}

	co_return std::unexpected{std::error_condition{EIO, std::generic_category()}};
}

Task<NetReadResultIp> NetManager::async_read_socket_raw(OpenSocketHandle sock)
{
	if (OpenSocketEntry* file = find_socket(sock))
	{
		co_return (co_await file->rx_queue.async_pop());
	}

	co_return std::unexpected{std::error_condition{EIO, std::generic_category()}};
}

Task<size_t> NetManager::async_write_socket(OpenSocketHandle sock, std::string bytes)
{
	if (bytes.size() == 0)
		co_return 0;
		
	if (OpenSocketEntry* file = find_socket(sock))
	{
		if (auto pak = make_tcp_reply_ip(sock, ip::TcpType::Data, std::move(bytes)))
		{
			size_t tx_size = pak->ByteSizeLong();
			send(std::move(*pak));
			co_return tx_size;
		}
	}
	co_return 0;
}

int32_t NetManager::listen(OpenSocketHandle sock)
{
	if (OpenSocketEntry* file = find_socket(sock))
	{
		return 0;
	}
	return 1;
}

Task<std::expected<OpenSocketPair, std::error_condition>> NetManager::async_accept_socket(OpenSocketHandle sock)
{
	while (true)
	{
		auto exp_raw = co_await async_read_socket_raw(sock);

		if (!exp_raw)
			co_return std::unexpected{exp_raw.error()};

		const ip::IpPackage& raw = *exp_raw;

		auto exp_src_addr = Address6::from_bytes(raw.src_ip());		// Remote, them
		auto exp_dest_addr = Address6::from_bytes(raw.dest_ip()); 	// Local, us

		if (!(exp_src_addr && exp_dest_addr))
			continue;

		ip::TcpPacket syn;
		if (!syn.ParseFromString(raw.payload()))
			continue;

		Address6 src_addr = *exp_src_addr;
		Address6 dest_addr = *exp_dest_addr;
		int32_t src_port = syn.src_port();
		int32_t dest_port = syn.dest_port();

		if (syn.type() == ip::TcpType::Syn)
		{
			auto exp_h = create_socket();
			if (!exp_h)
				co_return std::unexpected{exp_h.error()};

			OpenSocketEntry* acceptor = find_socket(sock);
			OpenSocketEntry* new_socket = find_socket(exp_h->first);
			new_socket->local_endpoint = acceptor->local_endpoint;
			new_socket->remote_endpoint = { src_addr, src_port };
			create_session(exp_h->first);

			if (auto opt_reply = make_tcp_reply_ip(exp_h->first, ip::TcpType::Ack, ""))
			{
				send(std::move(*opt_reply));
				co_return exp_h.value();
			}
		}
	}

	co_return std::unexpected{std::error_condition{EIO, std::generic_category()}};
}

void NetManager::send(ip::IpPackage&& package)
{
	assert(nic_);
	nic_->get_tx_queue().push(std::move(package));
}

void NetManager::send(ip::IpPackage&& package, Uid64 mac)
{
	assert(nic_);
	nic_->transfer(mac, std::move(package));
}

void NetManager::receive(ip::IpPackage&& package)
{
	const std::string& src_ip_bytes = package.src_ip();
	const std::string& dest_ip_bytes = package.dest_ip();

	auto exp_src_addr = Address6::from_bytes(src_ip_bytes.data());
	auto exp_dest_addr = Address6::from_bytes(dest_ip_bytes.data());

	if (exp_src_addr && exp_dest_addr)
	{
		receive(std::move(package), *exp_src_addr, *exp_dest_addr);
	}
}

void NetManager::receive(ip::IpPackage&& package, const Address6& src_addr, const Address6& dest_addr)
{
	const std::string& payload = package.payload();

	switch (package.protocol())
	{
		case ip::Protocol::ICMP:
		{
			ip::IcmpPacket icmp;
			if (icmp.ParseFromString(payload))
			{
				handle_packet(std::move(icmp), std::move(package), src_addr, dest_addr);
			}
			break;
		}
		case ip::Protocol::TCP:
		{
			ip::TcpPacket tcp;
			if (tcp.ParseFromString(payload))
			{
				handle_packet(std::move(tcp), std::move(package), src_addr, dest_addr);
			}
			break;
		}
		case ip::Protocol::UDP:
		{
			ip::UdpPacket udp;
			if (udp.ParseFromString(payload))
			{
				handle_packet(std::move(udp), std::move(package), src_addr, dest_addr);
			}
			break;
		}
		default: break;
	}
}

void NetManager::handle_packet(ip::IcmpPacket&& packet, ip::IpPackage&& outer, const Address6& src_addr, const Address6& dest_addr)
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

void NetManager::handle_packet(ip::TcpPacket&& packet, ip::IpPackage&& outer, const Address6& src_addr, const Address6& dest_addr)
{
	AddressPair src = { src_addr, packet.src_port() };
	AddressPair dest = { dest_addr, packet.dest_port() };
	AddressTuple sess = { dest, src };

	switch (packet.type())
	{
		case ip::TcpType::Data:
		{
			/* If this is a data transfer, we need to have an established session 
			-- use the full tuple. */
			if (OpenSocketEntry* sock = find_socket(sess))
			{
				sock->rx_queue.push(std::move(outer));
			}
			else
			{
				std::println("Dropping packet (no session).");
			}
			break;
		}
		case ip::TcpType::Fin:
		{
			std::println("Received FIN from {}.", src_addr);
			if (auto it = sessions_.find(sess); it != sessions_.end())
			{
				close_socket(it->second);
			}
			break;
		}
		default:
		{
			/* If this is a connection request or reply, check for a socket based on binding. */
			if (OpenSocketEntry* sock = find_socket(dest))
			{
				sock->rx_queue.push(std::move(outer));
			}
			else
			{
				std::println("Dropping packet (no binding).");
			}
			break;
		}
	}
}

void NetManager::handle_packet(ip::UdpPacket&& packet, ip::IpPackage&& outer, const Address6& src_addr, const Address6& dest_addr)
{
	AddressPair src = { src_addr, packet.src_port() };
	AddressPair dest = { dest_addr, packet.dest_port() };
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

bool NetManager::socket_is_open(OpenSocketHandle h) const
{
	return (sockets_.contains(h));
}

bool NetManager::socket_has_data(OpenSocketHandle h) const
{
	if (auto it = sockets_.find(h); it != sockets_.end())
	{
		const OpenSocketEntry& sock = it->second;
		bool no_data = (sock.rx_queue.empty() && sock.tx_queue.empty());
		return !no_data;
	}
	else return false;
}

bool NetManager::create_session(OpenSocketHandle h)
{
	if (OpenSocketEntry* file = find_socket(h))
	{
		const AddressPair& local = file->local_endpoint;
		const AddressPair& remote = file->remote_endpoint;
		sessions_.emplace(AddressTuple{local, remote}, h);
		std::println("Created session {}:{} <-> {}:{}.", local.addr, local.port, remote.addr, remote.port);
		return true;
	}

	return false;
}

OpenSocketEntry* NetManager::find_socket(OpenSocketHandle h)
{
	if (auto it_sock = sockets_.find(h); it_sock != sockets_.end())
		return &it_sock->second;

	return nullptr;
}

OpenSocketEntry* NetManager::find_socket(const AddressPair& tuple)
{
	if (auto it_h = bindings_.find(tuple); it_h != bindings_.end())
		return find_socket(it_h->second);

	return nullptr;
}

OpenSocketEntry* NetManager::find_socket(const AddressTuple& tuple)
{
	if (auto it_h = sessions_.find(tuple); it_h != sessions_.end())
		return find_socket(it_h->second);

	return nullptr;
}

std::optional<ip::TcpPacket> NetManager::make_tcp_query(OpenSocketHandle h, ip::TcpType type, std::string&& payload)
{
	OpenSocketEntry* entry = find_socket(h);
	if (entry == nullptr)
		return std::nullopt;

	const AddressPair& local = entry->local_endpoint;
	const AddressPair& remote = entry->remote_endpoint;

	ip::TcpPacket pak;
	pak.set_dest_port(local.port);
	pak.set_src_port(remote.port);
	pak.set_payload(std::move(payload));
	pak.set_type(type);
	return pak;
}

std::optional<ip::IpPackage> NetManager::make_tcp_query_ip(OpenSocketHandle h, ip::TcpType type, std::string&& payload)
{
	OpenSocketEntry* entry = find_socket(h);
	if (entry == nullptr)
		return std::nullopt;
		
	const AddressPair& local = entry->local_endpoint;
	const AddressPair& remote = entry->remote_endpoint;

	auto opt = make_tcp_query(h, type, std::move(payload));
	if (not opt)
		return std::nullopt;

	std::string tcp_str;
	opt->SerializeToString(&tcp_str);

	ip::IpPackage pak;
	pak.set_dest_ip(local.addr.raw);
	pak.set_src_ip(remote.addr.raw);
	pak.set_protocol(ip::Protocol::TCP);
	pak.set_payload(std::move(tcp_str));

	return pak;
}

std::optional<ip::TcpPacket> NetManager::make_tcp_reply(OpenSocketHandle h, ip::TcpType type, std::string&& payload)
{
	OpenSocketEntry* entry = find_socket(h);
	if (entry == nullptr)
		return std::nullopt;

	const AddressPair& local = entry->local_endpoint;
	const AddressPair& remote = entry->remote_endpoint;

	ip::TcpPacket pak;
	pak.set_dest_port(remote.port);
	pak.set_src_port(local.port);
	pak.set_payload(std::move(payload));
	pak.set_type(type);
	return pak;
}

std::optional<ip::IpPackage> NetManager::make_tcp_reply_ip(OpenSocketHandle h, ip::TcpType type, std::string&& payload)
{
	OpenSocketEntry* entry = find_socket(h);
	if (entry == nullptr)
		return std::nullopt;
		
	const AddressPair& local = entry->local_endpoint;
	const AddressPair& remote = entry->remote_endpoint;

	auto opt = make_tcp_reply(h, type, std::move(payload));
	if (not opt)
		return std::nullopt;

	std::string tcp_str;
	opt->SerializeToString(&tcp_str);

	ip::IpPackage pak;
	pak.set_dest_ip(remote.addr.raw);
	pak.set_src_ip(local.addr.raw);
	pak.set_protocol(ip::Protocol::TCP);
	pak.set_payload(std::move(tcp_str));

	return pak;
}

OpenSocketHandle NetManager::get_handle()
{
	if (free_handles_.empty())
		return handle_counter_++;

	return free_handles_.extract(free_handles_.begin()).value();
}

void NetManager::return_handle(OpenSocketHandle h)
{
	free_handles_.insert(h);
}

LinkUpdateAwaiter NetManager::async_await_link()
{
	return LinkUpdateAwaiter{nic_};
}

void NetManager::arp_request()
{
	nic_->broadcast([this](Uid64 mac, NIC* nic)
	{
		Address6 addr = nic->get_ip();
		arp_cache_[addr] = mac;
		std::println("- ARP entry: {} -> {}", addr, mac);
	});
}

void NetManager::arp_request(Uid64 mac)
{
	nic_->unicast(mac, [this](Uid64 mac, NIC* nic)
	{
		Address6 addr = nic->get_ip();
		arp_cache_[addr] = mac;
		std::println("- ARP entry: {} -> {}", addr, mac);
	});
}

void NetManager::link_unicast(Uid64 mac, NetCastFn unicast_fn)
{
	nic_->unicast(mac, std::move(unicast_fn));
}

void NetManager::link_broadcast(NetCastFn unicast_fn)
{
	nic_->broadcast(std::move(unicast_fn));
}

std::optional<Uid64> NetManager::arp_lookup(Address6 addr)
{
	if (auto it = arp_cache_.find(addr); it != arp_cache_.end())
	{
		return it->second;
	}
	return std::nullopt;
}
