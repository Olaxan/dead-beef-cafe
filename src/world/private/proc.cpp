#include "proc.h"
#include "os.h"
#include "task.h"

#include <coroutine>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>


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
		sid = host->set_sid();	
	}
	else
	{
		SessionManager* sess = owning_os->get_session_manager();
		sid = sess->create_session();
	}
	
	return sid;
}

int32_t Proc::get_sid() const
{
	return sid;
}

int32_t Proc::get_uid() const
{
	if (host) { return host->get_uid(); }

	SessionManager* sess = owning_os->get_session_manager();
	return sess->get_session_uid(sid);
}

int32_t Proc::get_gid() const
{
	if (host) { return host->get_gid(); }

	SessionManager* sess = owning_os->get_session_manager();
	return sess->get_session_gid(sid);
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
