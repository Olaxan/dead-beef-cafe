#include "timer_mgr.h"

#include <coroutine>
#include <print>

void TimerManager::step(float delta_seconds)
{
	for (std::size_t idx = 0; idx < timers_.size(); ++idx)
	{
		auto&& action = timers_[idx];

		if (!action.get_alive())
			continue;

		if (action.get_paused())
			continue;

		if (float& time = action.get_remaining().get() -= delta_seconds; time < 0.f)
		{
			//std::lock_guard<std::mutex> lock(mutex_);

			auto&& event = action.get_invoker().get();

			if (action.get_looping())
			{
				reset_timer_internal(idx);
			}
			else 
			{
				kill_timer_internal(idx);
			}

			std::invoke(event);
		}
	}
}

TimerHandle TimerManager::set_timer(float seconds, TimerCallbackFn event, bool looping)
{
	std::lock_guard<std::mutex> lock(mutex_);

	const std::size_t new_idx = [this]
	{
		if (!free_instances_.empty())
			return free_instances_.extract(std::begin(free_instances_)).value();

		std::size_t out = timers_.size();
		timers_.resize(out + 1);
		return out;
	}();

	auto&& timer = timers_[new_idx];
	timer.get_remaining().get() = seconds;
	timer.get_length().get() = seconds;
	timer.get_invoker().get() = std::move(event);
	timer.get_alive().get() = true;
	timer.get_looping().get() = looping;

	return { static_cast<int32_t>(new_idx) };
}

void TimerManager::pause_timer(const TimerHandle& handle)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (is_valid_handle(handle))
	{
		auto&& timer = timers_[handle.idx];
		timer.get_paused().get() = true;
	}
}

void TimerManager::cancel_timer(const TimerHandle &handle)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (is_valid_handle(handle))
	{
		kill_timer_internal(handle.idx);
	}
}

TimerAwaiter TimerManager::wait(float seconds)
{
	return TimerAwaiter{this, seconds};
}

void TimerManager::reset_timer_internal(std::size_t idx)
{
	auto&& timer = timers_[idx];
	float len = timer.get_length();
	timer.get_alive().get() = true;
	timer.get_remaining().get() = len;
}

void TimerManager::kill_timer_internal(std::size_t idx)
{
	auto&& timer = timers_[idx];
	timer.get_alive().get() = false;
	free_instances_.insert(idx);
}