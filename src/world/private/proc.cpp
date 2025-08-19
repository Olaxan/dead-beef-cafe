#include "proc.h"
#include "os.h"
#include "task.h"

#include <coroutine>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>


void Proc::set_leader(Proc* leader)
{
	host = leader;
	sid = leader->get_sid();
}

void Proc::dispatch(ProcessFn& program, std::vector<std::string> args, bool resume)
{
	this->args = std::move(args);
	task = std::move(std::invoke(program, *this, args));
	if (resume) task->handle.resume();
}

EagerTask<int32_t> Proc::await_dispatch(ProcessFn& program, std::vector<std::string> args)
{
	this->args = std::move(args);
	task = std::move(std::invoke(program, *this, this->args));
	task->handle.resume();
	co_return (co_await *task);
}

int32_t Proc::set_sid()
{
	sid = owning_os->create_sid();
	return sid;
}

int32_t Proc::get_sid() const
{
	return sid;
}

void Proc::set_uid(int32_t new_uid)
{
	uid = new_uid;
}

int32_t Proc::get_uid() const
{
	return uid;
}

void Proc::set_gid(int32_t new_gid)
{
	gid = new_gid;
}

int32_t Proc::get_gid() const
{
	return gid;
}

Proc* Proc::get_session_leader()
{
	return (host) ? host->get_session_leader() : this;
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
