#include "net_mgr.h"
#include "uuid.h"
#include "os.h"
#include "nic.h"
#include "filesystem.h"

#include "proto/ip_packet.pb.h"

#include <print>

NetManager::NetManager(OS* owner) : os_(*owner), nic_(owner->get_device<NIC>())
{ }

std::expected<uint64_t, std::runtime_error> NetManager::create_socket()
{

	FileSystem* fs = os_.get_filesystem();

	FilePath path{std::format("/run/{}", ++socket_index_)};
	FileSystem::CreateFileParams params{ .recurse = true };

	if (auto [fid, ptr, err] = fs->create_file(path, params); err == FileSystemError::Success)
	{
		sockets_[fid] = ptr;
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
	return 0;
}

ProcessReadAwaiter NetManager::async_read_socket(SocketDescriptor sock)
{
	if (auto&& it = sockets_.find(sock); it != sockets_.end())
		return ProcessReadAwaiter{it->second};

	return {};
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

File* NetManager::find_socket(const AddressPair& tuple)
{
	FileSystem* fs = os_.get_filesystem();

	if (auto it = bindings_.find(tuple); it != bindings_.end())
	{
		if (File* out = fs->find(it->second))
			return out;
	}

	return nullptr;
}
