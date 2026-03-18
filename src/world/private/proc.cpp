#include "proc.h"
#include "os.h"
#include "task.h"
#include "race_awaiter.h"
#include "proc_signal_awaiter.h"

#include <coroutine>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include <signal.h>


Proc::Proc(int32_t pid, OS* owner)
	: owning_os(owner), pid(pid) {}

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

Task<std::expected<std::string, std::error_condition>> Proc::read(EnvVarAccessMode mode)
{

	if (reader_)
	{
		auto res = co_await when_any(reader_(), ProcSignalAwaiter{this});
		if (res.index == 0)
		{
			co_return std::get<1>(res.value);
		}
		else co_return std::unexpected{std::error_condition{EINTR, std::generic_category()}};
	}

	if (host && mode == EnvVarAccessMode::Inherit)
	{
		auto res = co_await when_any(host->read(), ProcSignalAwaiter{this});
		if (res.index == 0)
		{
			co_return std::get<1>(res.value);
		}
		else co_return std::unexpected{std::error_condition{EINTR, std::generic_category()}};
	}

	co_return std::unexpected{std::error_condition{ENOSYS, std::generic_category()}};
}

void Proc::set_writer(WriterFn&& writer)
{
	writer_ = std::move(writer); 
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

Task<std::error_condition> Proc::wait(float seconds)
{
	auto res = co_await when_any(owning_os->wait(seconds), ProcSignalAwaiter{this});
	co_return (res.index == 0) ? std::error_condition{} : std::error_condition{EINTR, std::generic_category()};
}

void Proc::add_signal_callback(SignalCallbackFn&& fn)
{
	signal_callbacks_.push_back(std::move(fn));
}

void Proc::signal(SignalType sig)
{
	std::vector<SignalCallbackFn> empty_{};
	std::swap(signal_callbacks_, empty_);

	for (auto&& fn : empty_)
	{
		fn(sig);
	}
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

FileDescriptor Proc::get_descriptor()
{
	if (returned_descriptors_.empty())
		return descriptor_counter_++;

	auto h = returned_descriptors_.extract(returned_descriptors_.begin());
	return h.value();
}

void Proc::return_descriptor(FileDescriptor fs)
{
	returned_descriptors_.emplace(fs);
}

void Proc::copy_descriptors_from(const Proc& other)
{
	std::println("Copying descriptors from {}.", other.get_pid());
	fs.copy_descriptors_from(other.fs);
	net.copy_descriptors_from(other.net);
}

void Proc::enter()
{
	//std::println("enter({})", args.size() ? args[0] : "...");
	fs.register_descriptors();
	net.register_descriptors();
}

void Proc::exit()
{
	signal(SIGTERM);
	fs.close_all();
	net.close_all();
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
