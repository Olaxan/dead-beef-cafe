#include "netw.h"
#include "nic.h"
#include "net_mgr.h"


/* Begin SocketAcceptorAwaiter --- */
bool SocketAcceptAwaiter::await_ready()
{
	return true;
}

void SocketAcceptAwaiter::await_suspend(std::coroutine_handle<> h)
{
	h.resume();
}

int32_t SocketAcceptAwaiter::await_resume() const
{
	return 0;
}
/* End SocketAcceptorAwaiter --- */



/* Begin SocketConnectAwaiter --- */
SocketConnectAwaiter::SocketConnectAwaiter() = default;

SocketConnectAwaiter::SocketConnectAwaiter(NetManager* net, SocketDescriptor sock) 
: sock_(sock) { }

bool SocketConnectAwaiter::await_ready()
{
	return false;
}

void SocketConnectAwaiter::await_suspend(std::coroutine_handle<> h)
{
	h.resume();
}

int32_t SocketConnectAwaiter::await_resume() const
{
	return 0;
}
/* End SocketConnectAwaiter ---*/


/* Begin LinkUpdateAwaiter --- */
bool LinkUpdateAwaiter::await_ready()
{
	return false;
}

void LinkUpdateAwaiter::await_suspend(std::coroutine_handle<> h)
{
	nic_->add_link_update_callback([this, h](const LinkUpdatePair& link)
	{
		retval_ = link;
		h.resume();
	});
}

LinkUpdatePair LinkUpdateAwaiter::await_resume() const
{
	return retval_;
}
/* End LinkUpdateAwaiter --- */