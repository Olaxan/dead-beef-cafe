#pragma once

#include <set>
#include <coroutine>
#include <vector>

#include "soa_helpers.h"

class TimerManager;

using timer_callback_t = std::function<void(void)>;

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
	timer_callback_t, 
	uint8_t, 
	uint8_t,
	uint8_t>;

using TimerManagerContainer = BaseContainer<timer_container_t, DataLayout::SoA, TimerManagerDataTypes>;

struct TimerHandle
{
	int32_t idx{-1};
};

struct AsyncTimeAwaiter
{
	explicit AsyncTimeAwaiter(TimerManager* mgr, float seconds)
		: mgr_(mgr), time_(seconds) { }

	bool await_ready() const { return time_ == 0.f; }
	void await_suspend(std::coroutine_handle<> h) const;
	void await_resume() const {}

private:

	TimerManager* mgr_;
	float time_{0};

};

class TimerManager
{
public:

	TimerManager() = default;

	void step(float delta_seconds);

	/* Set a timer (in seconds), giving a timer handle back. 
	The event callback will be called once the timer reaches 0. */
	TimerHandle set_timer(float seconds, timer_callback_t event, bool looping = false);

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

	AsyncTimeAwaiter wait(float seconds) { return AsyncTimeAwaiter{this, seconds}; }

private:

	void reset_timer_internal(std::size_t idx);
	void kill_timer_internal(std::size_t idx);

	TimerManagerContainer timers_{};
	std::set<std::size_t> free_instances_{};
};