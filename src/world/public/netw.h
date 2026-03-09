#pragma once

#include "msg_queue.h"
#include "addr.h"
#include "uuid.h"

#include "proto/ip_packet.pb.h"

#include <cstdint>
#include <memory>
#include <coroutine>
#include <print>

class NIC;
class NetManager;

using SocketDescriptor = uint64_t;
using NetQueue = MessageQueue<ip::IpPackage>;
using NetMessageAwaiter = MessageQueueAwaiter<ip::IpPackage>;

struct SocketAcceptAwaiter
{
	explicit SocketAcceptAwaiter() { }

	SocketAcceptAwaiter(SocketAcceptAwaiter&) = delete;

	bool await_ready();
	void await_suspend(std::coroutine_handle<> h);
	int32_t await_resume() const;
	
};

struct SocketConnectAwaiter
{
	SocketConnectAwaiter();
	SocketConnectAwaiter(NetManager* net, SocketDescriptor sock);

	SocketConnectAwaiter(SocketConnectAwaiter&) = delete;

	bool await_ready();
	void await_suspend(std::coroutine_handle<> h);
	int32_t await_resume() const;

protected:

	SocketDescriptor sock_{0};
	
};

enum class LinkUpdateType : uint8_t
{
	LinkAdded,
	LinkRemoved,
	LinkIdle
};

using LinkUpdatePair = std::pair<UUID, LinkUpdateType>;
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