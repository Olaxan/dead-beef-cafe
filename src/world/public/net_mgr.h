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
class File;

using SocketDescriptor = uint64_t;

class NetManager
{
public:

	NetManager() = delete;
	NetManager(OS* owner);

	std::expected<uint64_t, std::runtime_error> create_socket();

	int32_t bind_socket(SocketDescriptor sock, AddressPair addr);

	ProcessReadAwaiter async_read_socket(SocketDescriptor sock);
	ProcessWriteAwaiter async_write_socket(SocketDescriptor sock, const std::string& bytes);
	
	void send(ip::IpPackage&& package);
	void receive(ip::IpPackage&& package);
	void receive(ip::IpPackage&& package, const Address6& addr);

	NetMessageAwaiter async_read_rx();
	NetMessageAwaiter async_read_tx();

	Address6 get_primary_ip() const;

protected:

	File* find_socket(const AddressPair& tuple);

	OS& os_;
	NIC* nic_{nullptr};

	uint64_t socket_index_{0};

	std::unordered_map<uint64_t, std::shared_ptr<File>> sockets_;
	std::unordered_map<AddressPair, uint64_t> bindings_;

};