#include "proc_netapi.h"

#include "os.h"
#include "net_mgr.h"

ProcNetApi::ProcNetApi(Proc* owner)
: owner_(owner), os_(owner_->owning_os), net_(os_->get_network_manager()) { }

ProcNetApi::~ProcNetApi() = default;

std::expected<FileDescriptor, std::error_condition> ProcNetApi::create_socket()
{
	Proc& proc = *owner_;

	if (auto exp_sock = net_->create_socket())
	{
		FileDescriptor fd = proc.get_descriptor();
		fd_table_[fd] = *exp_sock;
		return fd;
	}
	else return std::unexpected{exp_sock.error()};
}

std::error_condition ProcNetApi::close_socket(FileDescriptor sock)
{
	if (auto it = fd_table_.find(sock); it != fd_table_.end())
	{
		const OpenSocketPair& pair = it->second;
		OpenSocketHandle h = pair.first;
		fd_table_.erase(it);
		return net_->close_socket(h);
	}
	else return std::error_condition{EBADF, std::generic_category()};
}

std::error_condition ProcNetApi::bind_socket(FileDescriptor sock, AddressPair addr)
{
	if (auto it = fd_table_.find(sock); it != fd_table_.end())
	{
		const OpenSocketPair& pair = it->second;
		OpenSocketHandle h = pair.first;
		return net_->bind_socket(h, addr);
	}
	else return std::error_condition{EBADF, std::generic_category()};
}

std::error_condition ProcNetApi::bind_socket(FileDescriptor sock, Address6 addr, int32_t port)
{
	return bind_socket(sock, {addr, port});
}

Task<std::error_condition> ProcNetApi::async_connect_socket(FileDescriptor sock, AddressPair addr)
{
	if (auto it = fd_table_.find(sock); it != fd_table_.end())
	{
		const OpenSocketPair& pair = it->second;
		OpenSocketHandle h = pair.first;
		co_return (co_await net_->async_connect_socket(h, addr));
	}
	else co_return std::error_condition{EBADF, std::generic_category()};
}

Task<std::error_condition> ProcNetApi::async_connect_socket(FileDescriptor sock, Address6 addr, int32_t port)
{
	return async_connect_socket(sock, {addr, port});
}

Task<FileDescriptor> ProcNetApi::async_accept_socket(FileDescriptor sock)
{
	Proc& proc = *owner_;
	if (auto it = fd_table_.find(sock); it != fd_table_.end())
	{
		const OpenSocketPair& pair = it->second;
		OpenSocketHandle h = pair.first;
		auto exp_sock = co_await net_->async_accept_socket(h);
		FileDescriptor fd = proc.get_descriptor();
		fd_table_[fd] = *exp_sock;
		co_return fd;
	}
	else co_return -1;
}

Task<std::string> ProcNetApi::async_read_socket(FileDescriptor sock)
{
	if (auto it = fd_table_.find(sock); it != fd_table_.end())
	{
		const OpenSocketPair& pair = it->second;
		OpenSocketHandle h = pair.first;
		co_return (co_await net_->async_read_socket(h));
	}
	else co_return {};
}

Task<ip::TcpPacket> ProcNetApi::async_read_socket_tcp(FileDescriptor sock)
{
	if (auto it = fd_table_.find(sock); it != fd_table_.end())
	{
		const OpenSocketPair& pair = it->second;
		OpenSocketHandle h = pair.first;
		co_return (co_await net_->async_read_socket_tcp(h));
	}
	else co_return {};
}

Task<ip::IpPackage> ProcNetApi::async_read_socket_raw(FileDescriptor sock)
{
	if (auto it = fd_table_.find(sock); it != fd_table_.end())
	{
		const OpenSocketPair& pair = it->second;
		OpenSocketHandle h = pair.first;
		co_return (co_await net_->async_read_socket_raw(h));
	}
	else co_return {};
}

Task<size_t> ProcNetApi::async_write_socket(FileDescriptor sock, std::string bytes)
{
	if (auto it = fd_table_.find(sock); it != fd_table_.end())
	{
		const OpenSocketPair& pair = it->second;
		OpenSocketHandle h = pair.first;
		co_return (co_await net_->async_write_socket(h, std::move(bytes)));
	}
	else co_return 0;
}

int32_t ProcNetApi::listen(FileDescriptor sock)
{
	if (auto it = fd_table_.find(sock); it != fd_table_.end())
	{
		const OpenSocketPair& pair = it->second;
		OpenSocketHandle h = pair.first;
		return net_->listen(h);
	}
	else return 0;
}

bool ProcNetApi::socket_is_open(FileDescriptor sock) const
{
	if (auto it = fd_table_.find(sock); it != fd_table_.end())
	{
		const OpenSocketPair& pair = it->second;
		OpenSocketHandle h = pair.first;
		return net_->socket_is_open(h);
	}
	else return false;
}
