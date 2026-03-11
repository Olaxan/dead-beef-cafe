#include "proc_netapi.h"

#include "os.h"
#include "net_mgr.h"

ProcNetApi::ProcNetApi(Proc* owner)
: owner_(owner), os_(owner->owning_os), net_(os_->get_network_manager()) { }

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
