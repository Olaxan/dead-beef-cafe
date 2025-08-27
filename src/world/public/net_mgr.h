#pragma once

#include "addr.h"
#include "str_sock.h"
#include "msg_queue.h"

#include "proto/ip_packet.pb.h"

#include <string>
#include <expected>
#include <stdexcept>
#include <unordered_map>

class OS;

struct SocketDescriptor
{
	int32_t fd{0};
};

using NetQueue = MessageQueue<ip::IpPackage>;

class NetManager
{
public:

	NetManager() = delete;
	NetManager(OS* owner) : os_(*owner) { }

	std::expected<SocketDescriptor, std::runtime_error> create_socket();

	int32_t bind_socket(SocketDescriptor sock, AddressPair addr);
	
	void send(const SocketDescriptor& sock, ip::IpPackage&& package);

	NetQueue& get_rx_queue() { return rx_queue_; }
	NetQueue& get_tx_queue() { return tx_queue_; }

protected:

	OS& os_;

	NetQueue rx_queue_{};
	NetQueue tx_queue_{};

	std::unordered_map<int32_t, StringSocket> sockets_;
	std::unordered_map<AddressPair, int32_t> bindings_;

};