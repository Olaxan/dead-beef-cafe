#pragma once

#include "msg_queue.h"
#include "addr.h"

#include "proto/ip_packet.pb.h"

#include <cstdint>
#include <memory>
#include <coroutine>
#include <print>

class NIC;

using NetQueue = MessageQueue<ip::IpPackage>;
using NetMessageAwaiter = MessageQueueAwaiter<ip::IpPackage>;

struct SocketAcceptorAwaiter
{
	explicit SocketAcceptorAwaiter() { }

	SocketAcceptorAwaiter(SocketAcceptorAwaiter&) = delete;

	bool await_ready();
	void await_suspend(std::coroutine_handle<> h);
	int32_t await_resume() const;
	
};

enum class LinkUpdateType : uint8_t
{
	LinkAdded,
	LinkRemoved,
	LinkIdle
};

using LinkUpdatePair = std::pair<NIC*, LinkUpdateType>;
using LinkUpdateCallbackFn = std::move_only_function<void(const LinkUpdatePair&)>;

struct LinkUpdateAwaiter
{
	LinkUpdateAwaiter(NIC* nic)
		: nic_(nic) { }

    LinkUpdateAwaiter(LinkUpdateAwaiter&) = delete;
	LinkUpdateAwaiter(LinkUpdateAwaiter&&) = default;
	~LinkUpdateAwaiter() = default;

	bool await_ready();
	void await_suspend(std::coroutine_handle<> h);
	LinkUpdatePair await_resume() const;

private:

	NIC* nic_{nullptr};
	LinkUpdatePair retval_{};

};