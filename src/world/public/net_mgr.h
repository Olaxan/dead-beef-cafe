#pragma once

#include "addr.h"
#include "str_sock.h"
#include "proc_io.h"
#include "msg_queue.h"
#include "netw.h"

#include "proto/ip_packet.pb.h"

#include <string>
#include <expected>
#include <stdexcept>
#include <unordered_map>

class OS;
class NIC;

struct SocketDescriptor
{
	int32_t fd{0};
};

class NetManager
{
public:

	NetManager() = delete;
	NetManager(OS* owner);

	std::expected<SocketDescriptor, std::runtime_error> create_socket();

	int32_t bind_socket(SocketDescriptor sock, AddressPair addr);

	SocketReadAwaiter async_read_socket(SocketDescriptor sock);
	SocketWriteAwaiter async_write_socket(SocketDescriptor sock, const std::string& bytes);
	
	void send(ip::IpPackage&& package);
	void receive(ip::IpPackage&& package);
	void receive(ip::IpPackage&& package, const Address6& addr);

	NetMessageAwaiter async_read_rx();
	NetMessageAwaiter async_read_tx();

	Address6 get_primary_ip() const;

protected:

	StringSocket* find_socket(const AddressPair& tuple);

	OS& os_;
	NIC* nic_{nullptr};
	int32_t sock_fd_counter_{0};

	std::unordered_map<int32_t, StringSocket> sockets_;
	std::unordered_map<AddressPair, int32_t> bindings_;

};