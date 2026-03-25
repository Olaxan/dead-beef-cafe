#include "link_awaiter.h"

#include "nic.h"

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