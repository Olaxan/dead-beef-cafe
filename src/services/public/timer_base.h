#pragma once

#include "timer_awaiter.h"

#include <functional>

struct TimerHandle
{
	int32_t idx{-1};
};

using TimerCallbackFn = std::function<void(void)>;

class ITimerBase
{
public:

	/* Set a timer (in seconds), giving a timer handle back. 
	The event callback will be called once the timer reaches 0. */
	virtual TimerHandle set_timer(float seconds, TimerCallbackFn event, bool looping = false) = 0;

	/* Pause a set timer without cancelling it. 
	If the handle is invalid, nothing happens. */
	virtual void pause_timer(const TimerHandle& handle) = 0;

	/* Cancel a timer.
	If the handle is invalid, nothing happens. */
	virtual void cancel_timer(const TimerHandle& handle) = 0;

	/* Check if this is a valid timer handle.*/
	virtual bool is_valid_handle(const TimerHandle& handle) = 0;

	/* Wait for this timer using a coroutine awaiter. */
	virtual TimerAwaiter wait(float seconds) = 0;

};