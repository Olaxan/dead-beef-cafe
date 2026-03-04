#include "net_mgr.h"
#include "uuid.h"
#include "os.h"
#include "nic.h"

#include "proto/ip_packet.pb.h"

#include <print>

NetManager::NetManager(OS* owner) : os_(*owner), nic_(owner->get_device<NIC>())
{ }

std::expected<SocketDescriptor, std::runtime_error> NetManager::create_socket()
{
	int32_t fd = ++sock_fd_counter_;
	sockets_.emplace(fd);
	return SocketDescriptor{fd};
}

int32_t NetManager::bind_socket(SocketDescriptor sock, AddressPair addr)
{
	if (bindings_.contains(addr))
		return 1;

	bindings_[addr] = sock.fd;
	return 0;
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
	if (StringSocket* sock = find_socket(pair))
	{
		sock->receive(std::forward<ip::IpPackage>(package));
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

StringSocket* NetManager::find_socket(const AddressPair& tuple)
{
	if (auto ibind = bindings_.find(tuple); ibind != bindings_.end())
	{
		if (auto isock = sockets_.find(ibind->second); isock != sockets_.end())
		{
			return &isock->second;
		}
	}

	return nullptr;
}
