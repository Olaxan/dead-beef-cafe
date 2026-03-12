#pragma once

#include "task.h"
#include "addr.h"
#include "proc_types.h"
#include "net_types.h"

#include "proto/ip_packet.pb.h"
#include "proto/tcp_packet.pb.h"

#include <cstdint>
#include <memory>
#include <expected>
#include <system_error>
#include <set>

class OS;
class Proc;
class NetManager;

class ProcNetApi
{
public:

	ProcNetApi() = delete;
	ProcNetApi(Proc* owner);
	~ProcNetApi();

	std::expected<FileDescriptor, std::error_condition> create_socket();
	std::error_condition close_socket(FileDescriptor sock);

	std::error_condition bind_socket(FileDescriptor sock, AddressPair addr);
	std::error_condition bind_socket(FileDescriptor sock, Address6 addr, int32_t port);

	Task<std::error_condition> async_connect_socket(FileDescriptor sock, AddressPair addr);
	Task<std::error_condition> async_connect_socket(FileDescriptor sock, Address6 addr, int32_t port);
	
	Task<FileDescriptor> async_accept_socket(FileDescriptor sock);

	Task<std::string> async_read_socket(FileDescriptor sock);
	Task<ip::TcpPacket> async_read_socket_tcp(FileDescriptor sock);
	Task<ip::IpPackage> async_read_socket_raw(FileDescriptor sock);

	Task<size_t> async_write_socket(FileDescriptor sock, std::string bytes);
	
	int32_t listen(FileDescriptor sock);

	bool socket_is_open(FileDescriptor sock) const;

protected:

	Proc* owner_{nullptr};
	OS* os_{nullptr};
	NetManager* net_{nullptr};

	std::unordered_map<FileDescriptor, OpenSocketPair> fd_table_{};

};
