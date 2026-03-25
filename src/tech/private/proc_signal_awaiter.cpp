#include "proc_signal_awaiter.h"

#include "proc.h"

ProcSignalAwaiter::ProcSignalAwaiter(Proc* proc)
	: proc_(proc) {}

ProcSignalAwaiter::~ProcSignalAwaiter()
{
	//proc_.reset();
};

void ProcSignalAwaiter::await_suspend(std::coroutine_handle<> h)
{
	assert(proc_);
	proc_->add_signal_callback([this, h](SignalType sig)
	{
		signal_ = sig;
		h.resume();
	});
}