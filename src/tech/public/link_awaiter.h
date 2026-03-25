#pragma once

#include "uid64.h"

#include <cstdint>
#include <functional>
#include <coroutine>
#include <memory>

class NIC;

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