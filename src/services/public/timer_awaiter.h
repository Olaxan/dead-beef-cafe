#pragma once

#include <coroutine>

class ITimerBase;

struct TimerAwaiter
{
	explicit TimerAwaiter(ITimerBase* mgr, float seconds);

	bool await_ready() const;
	void await_suspend(std::coroutine_handle<> h) const;
	float await_resume() const;

private:

	ITimerBase* mgr_;
	float time_{0};

};