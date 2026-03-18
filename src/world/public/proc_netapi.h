#pragma once

#include "task.h"
#include "addr.h"
#include "proc_types.h"
#include "net_types.h"
#include "filesystem_types.h"

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

	void copy_descriptors_from(const ProcNetApi& other);
	void register_descriptors();

	std::expected<FileDescriptor, std::error_condition> create_socket();
	std::error_condition close_socket(FileDescriptor sock);
	Task<std::error_condition> async_close_socket(FileDescriptor fd);

	std::error_condition bind_socket(FileDescriptor sock, AddressPair addr);
	std::error_condition bind_socket(FileDescriptor sock, Address6 addr, int32_t port);

	Task<std::error_condition> async_connect_socket(FileDescriptor sock, AddressPair addr);
	Task<std::error_condition> async_connect_socket(FileDescriptor sock, Address6 addr, int32_t port);
	
	Task<FileDescriptor> async_accept_socket(FileDescriptor sock);

	Task<NetReadResult> async_read_socket(FileDescriptor sock) const;
	Task<NetReadResultTcp> async_read_socket_tcp(FileDescriptor sock) const;
	Task<NetReadResultIp> async_read_socket_raw(FileDescriptor sock) const;

	Task<size_t> async_write_socket(FileDescriptor sock, std::string bytes) const;
	
	int32_t listen(FileDescriptor sock);

	bool socket_is_open(FileDescriptor sock) const;
	Address6 get_primary_ip() const;

	void close_all();

protected:

	Proc* owner_{nullptr};
	OS* os_{nullptr};
	NetManager* net_{nullptr};

	std::unordered_map<FileDescriptor, OpenSocketPair> fd_table_{};

};
