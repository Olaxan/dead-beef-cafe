#pragma once

#include "task.h"
#include "addr.h"

#include <cstdint>
#include <memory>
#include <expected>
#include <system_error>
#include <unordered_set>

class OS;
class Proc;
class NetManager;

class ProcNetApi
{
public:

	ProcNetApi() = delete;
	ProcNetApi(Proc* owner);
	~ProcNetApi();

	std::expected<FileDescriptor, std::runtime_error> create_socket();
	std::error_condition close_socket(FileDescriptor sock);

	int32_t bind_socket(FileDescriptor sock, AddressPair addr);
	int32_t bind_socket(FileDescriptor sock, Address6 addr, int32_t port);

	Task<int32_t> async_connect_socket(FileDescriptor sock, AddressPair addr);
	Task<int32_t> async_connect_socket(FileDescriptor sock, Address6 addr, int32_t port);
	
	Task<FileDescriptor> async_accept_socket(FileDescriptor sock);

	Task<std::string> async_read_socket(FileDescriptor sock);
	Task<ip::TcpPacket> async_read_socket_tcp(FileDescriptor sock);
	Task<ip::IpPackage> async_read_socket_raw(FileDescriptor sock);

	Task<size_t> async_write_socket(FileDescriptor sock, std::string bytes);
	
	int32_t listen(FileDescriptor sock);

protected:

	Proc* owner_{nullptr};
	OS* os_{nullptr};
	NetManager* net_{nullptr};

	std::set<FileDescriptor> open_sockets_{};

};
