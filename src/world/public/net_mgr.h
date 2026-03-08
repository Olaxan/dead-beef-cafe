#pragma once

#include "addr.h"
#include "file_socket.h"
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
class File;

using SocketDescriptor = uint64_t;

class NetManager
{
public:

	NetManager() = delete;
	NetManager(OS* owner);
	~NetManager() = default;

	std::expected<SocketDescriptor, std::runtime_error> create_socket();

	int32_t bind_socket(SocketDescriptor sock, AddressPair addr);

	void connect_socket(SocketDescriptor sock, AddressPair addr);
	void connect_socket(SocketDescriptor sock, Address6 addr, int32_t port);


	ProcessReadAwaiter async_read_socket(SocketDescriptor sock);
	void async_write_socket(SocketDescriptor sock, const std::string& bytes);
	
	void send(ip::IpPackage&& package);
	void receive(ip::IpPackage&& package);
	void receive(ip::IpPackage&& package, const Address6& addr);

	NetMessageAwaiter async_read_rx();
	NetMessageAwaiter async_read_tx();

	Address6 get_primary_ip() const;

	bool socket_is_open(SocketDescriptor fd) const;
	
	void process_sockets();

protected:

	SocketFile* find_socket(SocketDescriptor sock_fd);
	SocketFile* find_socket(const AddressPair& tuple);

	OS& os_;
	NIC* nic_{nullptr};

	uint64_t socket_index_{0};

	std::unordered_map<SocketDescriptor, std::shared_ptr<SocketFile>> sockets_;
	std::unordered_map<AddressPair, SocketDescriptor> bindings_;

};