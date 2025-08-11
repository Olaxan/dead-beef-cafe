#include "proc.h"
#include "os.h"
#include "task.h"

#include <coroutine>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

void Proc::dispatch(ProcessFn &program, std::vector<std::string> args, bool resume)
{
	this->args = std::move(args);
	task = std::move(std::invoke(program, *this, args));
	if (resume) task->handle.resume();
}

EagerTask<int32_t> Proc::await_dispatch(ProcessFn &program, std::vector<std::string> args)
{
	this->args = std::move(args);
	task = std::move(std::invoke(program, *this, this->args));
	task->handle.resume();
	co_return (co_await *task);
}

int ProcCoutBuf::sync()
{
	if (this->in_avail() == 0)
		return 0;

	proc_->put("{}", this->str());
	this->str("");
	return 0;
}

int ProcCerrBuf::sync()
{
	if (this->in_avail() == 0)
		return 0;

	proc_->err("{}", this->str());
	this->str("");
	return 0;
}
