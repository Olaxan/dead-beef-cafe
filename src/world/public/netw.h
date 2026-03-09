#pragma once

#include "msg_queue.h"
#include "addr.h"

#include "proto/ip_packet.pb.h"

#include <cstdint>
#include <memory>
#include <coroutine>
#include <print>

using NetQueue = MessageQueue<ip::IpPackage>;
using NetMessageAwaiter = MessageQueueAwaiter<ip::IpPackage>;

struct SocketAcceptorAwaiter
{
	explicit SocketAcceptorAwaiter() { }

	SocketAcceptorAwaiter(SocketAcceptorAwaiter&) = delete;

	bool await_ready()
	{
		return true;
	}

	void await_suspend(std::coroutine_handle<> h)
	{
		h.resume();
	}

	int32_t await_resume() const
	{
		return 0;
	}
};
