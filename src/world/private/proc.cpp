#include "proc.h"
#include "os.h"
#include "task.h"

#include <coroutine>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

const SessionData& Proc::get_session() const
{
	if (host) return host->get_session();
	return sess_;
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
	if (host)
	{
		sess_.sid = host->set_sid();
	}
	else
	{
		sess_.sid = 1;
	}

	return sess_.sid;
}

int32_t Proc::set_uid(int32_t new_uid)
{
	if (host)
	{
		sess_.uid = new_uid;
		return host->set_uid(new_uid);
	}
	sess_.uid = new_uid;
	return 0;
}

int32_t Proc::set_gid(int32_t new_gid)
{
	if (host)
	{
		sess_.gid = new_gid;
		return host->set_gid(new_gid);
	}
	sess_.gid = new_gid;
	return 0;
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
