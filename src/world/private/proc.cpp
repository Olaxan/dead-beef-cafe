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
	sid = (host) ? host->set_sid() : owning_os->create_session();
	std::println("Process {} sid = {}.", pid, sid);
	return sid;
}

bool Proc::set_uid(int32_t new_uid)
{
	return owning_os->set_session_uid(sid, new_uid);
}

bool Proc::set_gid(int32_t new_gid)
{
	return owning_os->set_session_gid(sid, new_gid);
}

SessionData Proc::get_session() const
{
	if (host)
		return host->get_session();
		
	if (std::optional<SessionData> sess = owning_os->get_session(sid); sess.has_value())
		return *sess;
	
	return SessionData();
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
