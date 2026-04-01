#pragma once

#include "proc_types.h"

#include <coroutine>
#include <memory>
#include <cstdint>

class Proc;
class ProcSignalAwaiter
{
public:

	ProcSignalAwaiter(const Proc* proc); 

	//ProcSignalAwaiter(ProcSignalAwaiter&) = delete;

	~ProcSignalAwaiter();

	bool await_ready() const { return false; }
	void await_suspend(std::coroutine_handle<> h);
	SignalType await_resume() const { return signal_; }

protected:

	const Proc* proc_{nullptr};
	SignalType signal_{0};

};