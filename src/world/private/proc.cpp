#include "proc.h"
#include "os.h"
#include "task.h"

#include <coroutine>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>


Proc::~Proc()
{
	s_err << std::flush;
	s_out << std::flush;
}

void Proc::set_leader(Proc* leader)
{
	host = leader;
	sid = leader->get_sid();
}

std::string_view Proc::get_var(std::string key, EnvVarAccessMode mode) const
{
	if (auto it = envvars_.find(key); it != envvars_.end())
		return it->second;

	if (host && mode == EnvVarAccessMode::Inherit)
		return host->get_var(key, mode);
	
	return {};
}

std::string_view Proc::get_var_or(std::string key, std::string_view or_value, EnvVarAccessMode mode) const
{
	std::string_view get = get_var(key, mode);
	return !get.empty() ? get : or_value;
}

void Proc::set_reader(ReaderFn&& reader)
{
	reader_ = std::move(reader);
}

ProcessReadAwaiter Proc::read(EnvVarAccessMode mode)
{
	if (reader_)
		return reader_();

	if (host && mode == EnvVarAccessMode::Inherit)
		return host->read(mode);

	return ProcessReadAwaiter{nullptr};
}

void Proc::set_writer(WriterFn writer)
{
	writer_ = writer; 
}

bool Proc::write(const std::string& msg)
{
	if (writer_)
	{
		writer_(msg);
		return true;
	}

	if (host)
		return host->write(msg);

	return false;
}

bool Proc::write(const char* msg)
{
	return write(std::string(msg));
}

bool Proc::write(const com::CommandQuery& com)
{
	std::string out;
	com.SerializeToString(&out);
	return write(out);
}

bool Proc::write(const com::CommandReply& com)
{
	std::string out;
	com.SerializeToString(&out);
	return write(out);
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
