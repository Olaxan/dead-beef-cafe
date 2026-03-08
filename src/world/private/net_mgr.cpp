#include "net_mgr.h"
#include "uuid.h"
#include "os.h"
#include "host.h"
#include "nic.h"
#include "filesystem.h"

#include "proto/ip_packet.pb.h"

#include <print>
#include <memory>

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

void NetManager::connect_socket(SocketDescriptor sock, AddressPair addr)
{
	if (SocketFile* file = find_socket(sock))
		file->other_endpoint = addr;
}

void NetManager::connect_socket(SocketDescriptor sock, Address6 addr, int32_t port)
{
	return connect_socket(sock, {addr, port});
}

ProcessReadAwaiter NetManager::async_read_socket(SocketDescriptor sock)
{
	if (auto&& it = sockets_.find(sock); it != sockets_.end())
		return ProcessReadAwaiter{it->second};

	return {};
}

void NetManager::async_write_socket(SocketDescriptor sock, const std::string& bytes)
{
	if (auto&& it = sockets_.find(sock); it != sockets_.end())
		it->second->write(bytes);
}

void NetManager::send(ip::IpPackage&& package)
{
	assert(nic_);
	nic_->get_tx_queue().push(std::move(package));
}

void NetManager::receive(ip::IpPackage&& package)
{
	const std::string& dest_ip_bytes = package.dest_ip();
	if (auto exp_dest_ip = Address6::from_bytes(dest_ip_bytes.data()))
	{
		receive(std::forward<ip::IpPackage>(package), *exp_dest_ip);
	}
}

void NetManager::receive(ip::IpPackage&& package, const Address6& addr)
{
	AddressPair pair = { addr, package.dest_port() };
	if (File* sock = find_socket(pair))
	{
		sock->write(package.payload());
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

void NetManager::process_sockets()
{
	for (auto& [fd, sock] : sockets_)
	{
		if (std::optional<std::string> data = sock->eat(); data.has_value())
		{
			const AddressPair& src = sock->local_endpoint;
			const AddressPair& dest = sock->other_endpoint;

			ip::IpPackage pak;
			pak.set_dest_port(dest.port);
			pak.set_dest_ip(dest.addr.raw);
			pak.set_src_port(src.port);
			pak.set_src_ip(src.addr.raw);

			send(std::move(pak));
		}
	}
}
