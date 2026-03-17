#include "dbc.h"

#include <thread>

#include <asio.hpp>

std::string DbcUtils::make_string(asio::streambuf& streambuf)
{
	return { asio::buffers_begin(streambuf.data()), asio::buffers_end(streambuf.data()) };
}

void IoServiceAwaiter::await_suspend(std::coroutine_handle<> h)
{
	std::jthread runner([this, h] 
	{ 
		count_ = srv_.run();
		h.resume();
	});
}
