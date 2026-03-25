#pragma once

#include "timer_base.h"
#include "timer_awaiter.h"
#include "soa_helpers.h"
#include "task.h"

#include <set>
#include <coroutine>
#include <vector>
#include <mutex>

class TimerManager;

template<typename T>
using timer_container_t = std::vector<T, std::allocator<T>>;

enum TimerManagerComponents : uint8_t
{
	ComTimerLengthSeconds,
    ComTimerSecondsRemaining,
    ComTimerInvoker,
	ComTimerAlive,
	ComTimerPaused,
	ComTimerLooping
};

template<typename ... T>
struct TimerManagerData : public std::tuple<T ...>
{
	using std::tuple<T...>::tuple;
 	auto& get_length() { return std::get<ComTimerLengthSeconds>(*this); }
	auto& get_remaining() { return std::get<ComTimerSecondsRemaining>(*this); }
 	auto& get_invoker() { return std::get<ComTimerInvoker>(*this); }
	auto& get_alive() { return std::get<ComTimerAlive>(*this); }
	auto& get_paused() { return std::get<ComTimerPaused>(*this); }
	auto& get_looping() { return std::get<ComTimerLooping>(*this); }
};

using TimerManagerDataTypes = TimerManagerData<
	float,
	float,
	TimerCallbackFn, 
	uint8_t, 
	uint8_t,
	uint8_t>;

using TimerManagerContainer = BaseContainer<timer_container_t, DataLayout::SoA, TimerManagerDataTypes>;

class TimerManager : public ITimerBase
{
public:

	TimerManager() = default;

	void step(float delta_seconds);

	/* Set a timer (in seconds), giving a timer handle back. 
	The event callback will be called once the timer reaches 0. */
	TimerHandle set_timer(float seconds, TimerCallbackFn event, bool looping = false) override;

	/* Pause a set timer without cancelling it. 
	If the handle is invalid, nothing happens. */
	void pause_timer(const TimerHandle& handle);

	/* Cancel a timer.
	If the handle is invalid, nothing happens. */
	void cancel_timer(const TimerHandle& handle);

	bool is_valid_handle(const TimerHandle& handle)
	{
		return (handle.idx >= 0 && handle.idx < static_cast<int32_t>(timers_.size()));
	};

	TimerAwaiter wait(float seconds);

private:

	void reset_timer_internal(std::size_t idx);
	void kill_timer_internal(std::size_t idx);

	mutable std::mutex mutex_{};
	TimerManagerContainer timers_{};
	std::set<std::size_t> free_instances_{};
};