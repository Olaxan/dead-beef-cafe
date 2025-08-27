#include "net_mgr.h"
#include "uuid.h"

#include "proto/ip_packet.pb.h"

#include <print>

std::expected<SocketDescriptor, std::runtime_error> NetManager::create_socket()
{
	return SocketDescriptor{1};
}

void NetManager::send(const SocketDescriptor& sock, ip::IpPackage&& package)
{
	tx_queue_.push(std::move(package));
}