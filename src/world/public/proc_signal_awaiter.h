#pragma once

#include "proc_types.h"

#include <coroutine>
#include <memory>
#include <cstdint>

class Proc;
class ProcSignalAwaiter
{
public:

	ProcSignalAwaiter(Proc* proc); 

	ProcSignalAwaiter(Proc&) = delete;

	~ProcSignalAwaiter();

	bool await_ready() const { return false; }
	void await_suspend(std::coroutine_handle<> h);
	SignalType await_resume() const { return signal_; }

protected:

	Proc* proc_{nullptr};
	SignalType signal_{0};

};