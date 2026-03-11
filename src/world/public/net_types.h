#pragma once

#include "msg_queue.h"
#include "addr.h"
#include "uuid.h"

#include "proto/ip_packet.pb.h"
#include "proto/tcp_packet.pb.h"

#include <cstdint>
#include <memory>
#include <coroutine>
#include <functional>
#include <print>


class NIC;
class NetManager;
class SocketFile;

using SocketDescriptor = int64_t;
using NetQueue = MessageQueue<ip::IpPackage>;
using NetMessageAwaiter = MessageQueueAwaiter<ip::IpPackage>;

enum class LinkUpdateType : uint8_t
{
	LinkAdded,
	LinkRemoved,
	LinkIdle
};

using LinkUpdatePair = std::pair<Uid64, LinkUpdateType>;
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