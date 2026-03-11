#include "proc_netapi.h"

#include "os.h"
#include "net_mgr.h"

ProcNetApi::ProcNetApi(Proc* owner)
: owner_(owner), os_(owner_->owning_os), net_(os_->get_network_manager()) { }

ProcNetApi::~ProcNetApi() = default;

std::expected<SocketDescriptor, std::runtime_error> ProcNetApi::create_socket()
{
	auto ret = net_->create_socket();
	if (ret) { open_sockets_.insert(ret.value()); }
	return ret;
}

std::error_condition ProcNetApi::close_socket(FileDescriptor sock)
{
	open_sockets_.erase(sock);
	return net_->close_socket(sock);
}

int32_t ProcNetApi::bind_socket(FileDescriptor sock, AddressPair addr)
{
	return net_->bind_socket(sock, addr);
}

int32_t ProcNetApi::bind_socket(FileDescriptor sock, Address6 addr, int32_t port)
{
	return net_->bind_socket(sock, addr, port);
}

Task<int32_t> ProcNetApi::async_connect_socket(FileDescriptor sock, AddressPair addr)
{
	return net_->async_connect_socket(sock, addr);
}

Task<int32_t> ProcNetApi::async_connect_socket(FileDescriptor sock, Address6 addr, int32_t port)
{
	return net_->async_connect_socket(sock, addr, port);
}

Task<FileDescriptor> ProcNetApi::async_accept_socket(FileDescriptor sock)
{
	return net_->async_accept_socket(sock);
}

Task<std::string> ProcNetApi::async_read_socket(FileDescriptor sock)
{
	return net_->async_read_socket(sock);
}

Task<ip::TcpPacket> ProcNetApi::async_read_socket_tcp(FileDescriptor sock)
{
	return net_->async_read_socket_tcp(sock);
}

Task<ip::IpPackage> ProcNetApi::async_read_socket_raw(FileDescriptor sock)
{
	return net_->async_read_socket_raw(sock);
}

Task<size_t> ProcNetApi::async_write_socket(FileDescriptor sock, std::string bytes)
{
	return net_->async_write_socket(sock, bytes);
}

int32_t ProcNetApi::listen(FileDescriptor sock)
{
	return net_->listen(sock);
}
