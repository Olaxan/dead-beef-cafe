#include "timer_awaiter.h"
#include "timer_base.h"

TimerAwaiter::TimerAwaiter(ITimerBase* mgr, float seconds)
	: mgr_(mgr), time_(seconds) { }

bool TimerAwaiter::await_ready() const
{
    return false;
}

float TimerAwaiter::await_resume() const
{
    return 0.0f;
}

void TimerAwaiter::await_suspend(std::coroutine_handle<> h) const
{
	mgr_->set_timer(time_, [h]{ h.resume(); });
}