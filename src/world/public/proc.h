#pragma once

#include "task.h"
#include "proc_types.h"
#include "proc_fsapi.h"
#include "proc_netapi.h"
#include "proc_sysapi.h"
#include "term_utils.h"
#include "session.h"
#include "timer_mgr.h"

#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <print>
#include <coroutine>
#include <optional>
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <ostream>

class Proc;
class OS;

using WriterFn = std::function<void(const std::string&)>;
using ReaderFn = std::move_only_function<Task<std::string>(void)>;

enum class EnvVarAccessMode
{
	Local,
	Inherit
};

class ProcCoutBuf : public std::stringbuf
{
public:
	ProcCoutBuf(Proc* proc)
		: proc_(proc) {}

	virtual int sync() override;

protected:
	Proc* proc_;

};

class ProcCerrBuf : public std::stringbuf
{
public:
	ProcCerrBuf(Proc* proc)
		: proc_(proc) {}

	virtual int sync() override;

protected:
	Proc* proc_;

};

class Proc
{
public:

	Proc(int32_t pid, OS* owner);
	Proc(Proc&) = delete;

	~Proc();

	void set_leader(Proc* leader);

	std::string_view get_name() const { return args.empty() ?  "?" : std::string_view(args[0]); }

	/* --- FUNCTIONS THAT RELATE TO ENVIRONMENT VARIABLES ---- */

	/* Sets an environment variable in the process. */
	template<typename T>
	void set_var(std::string key, T val, EnvVarAccessMode mode = EnvVarAccessMode::Inherit)
	{
		envvars_[key] = std::format("{}", val);
		
		if (host && mode == EnvVarAccessMode::Inherit)
			host->set_var(key, val, mode);
	}

	/* Reads the value of an environment variable in the process, or empty if not found. */
	template<typename T>
	T get_var(std::string key, EnvVarAccessMode mode = EnvVarAccessMode::Inherit) const
	{
		if (auto it = envvars_.find(key); it != envvars_.end())
		{
			std::stringstream ss;
			ss << it->second;
			T ret;
			ss >> ret;
			return ret;
		}

		if (host && mode == EnvVarAccessMode::Inherit)
			return host->get_var<T>(key, mode);
		
		return {};
	}

	/* Default version of get_var, returning a string (no converstion). */
	std::string_view get_var(std::string key, EnvVarAccessMode mode = EnvVarAccessMode::Inherit) const;

	std::string_view get_var_or(std::string key, std::string_view or_value, EnvVarAccessMode mode = EnvVarAccessMode::Inherit) const;


	/* ---- FUNCTIONS THAT RELATE TO GETTING INPUT FROM THE USER --- */

	/* Add a reader of a certain type, which can be used in read function calls. */
    void set_reader(ReaderFn&& reader);

	/* Try to read something from a reader function, if it has been provided. 
	The data type must be specified and match, or an exception will be raised. */
	Task<std::string> read(EnvVarAccessMode mode = EnvVarAccessMode::Inherit);


	/* ---- FUNCTIONS THAT RELATE TO PUTTING THINGS ON THE TERMINAL --- */

	/* Register a writer of a certain type, to be used in put/write function calls instead of standard out. */
    void set_writer(WriterFn writer);

	/* Templated function of put/warn/err which allows to write any kind of data to writer map. */
    bool write(const std::string& msg);

	/* Overload to ensure string literals work. */
    bool write(const char* msg);
	bool write(const com::CommandQuery& com);
	bool write(const com::CommandReply& com);


	/* Write to the process 'standard output'. */
	template<typename ...Args>
	void put(std::format_string<Args...> fmt, Args&& ...args)
	{
		write(std::format(fmt, std::forward<Args>(args)...));
	}

	/* Write to the process 'standard output', with  a \n at the end. */
	template<typename ...Args>
	void putln(std::format_string<Args...> fmt, Args&& ...args)
	{
		write(std::format(fmt, std::forward<Args>(args)...).append("\n"));
	}

	/* Write to the process 'standard output', only with yellow ANSI colours. */
	template<typename ...Args>
	void warn(std::format_string<Args...> fmt, Args&& ...args)
	{
		put("(" CSI_CODE(33) "!" CSI_RESET ") {0}", std::format(fmt, std::forward<Args>(args)...));
	}

	/* Write to the process 'standard output', with yellow ANSI colours, with  a \n at the end. */
	template<typename ...Args>
	void warnln(std::format_string<Args...> fmt, Args&& ...args)
	{
		putln("(" CSI_CODE(33) "!" CSI_RESET ") {0}", std::format(fmt, std::forward<Args>(args)...));
	}

	/* Write to the process 'standard output', only with red ANSI colours. */
	template<typename ...Args>
	void err(std::format_string<Args...> fmt, Args&& ...args)
	{
		put("(" CSI_CODE(31) "!" CSI_RESET ") {0}", std::format(fmt, std::forward<Args>(args)...));
	}

	/* Write to the process 'standard output', with red ANSI colours, with  a \n at the end. */
	template<typename ...Args>
	void errln(std::format_string<Args...> fmt, Args&& ...args)
	{
		putln("(" CSI_CODE(31) "!" CSI_RESET ") {0}", std::format(fmt, std::forward<Args>(args)...));
	}


	/* --- FUNCTIONS THAT RELATE TO EXECUTING SUB-PROCESSES --- */

	/* Set this process running with a function and function arguments. 
	Optionally resume the provided coroutine immediately, if it is lazy. */
	void dispatch(ProcessFn& program, std::vector<std::string> args, bool resume = true);

	/* Variant of 'dispatch' which awaits its own hosted process task. */
	EagerTask<int32_t> await_dispatch(ProcessFn& program, std::vector<std::string> args);


	/* --- FUNCTIONS THAT RELATE TO OS --- */
	[[nodiscard]] TimerAwaiter wait(float seconds);


	/* --- FUNCTIONS THAT RELATE TO SESSION --- */

	int32_t get_pid() const { return pid; }

	int32_t set_sid();
	int32_t get_sid() const;

	void set_uid(int32_t new_uid);
	int32_t get_uid() const;

	void set_gid(int32_t new_gid);
	int32_t get_gid() const;

	Proc* get_session_leader();

	/* --- FUNCTIONS THAT RELATE TO FILE DESCRIPTORS --- */

	FileDescriptor get_descriptor();
	void return_descriptor(FileDescriptor fs);

	/* --- PUBLICALLY ACCESSIBLE API:s --- */

public:

	Proc* host{nullptr};
	OS* owning_os{nullptr};
	int32_t pid{0};

	int32_t sid{0};
	int32_t uid{0};
	int32_t gid{0};

	ProcFsApi fs{this};
	ProcNetApi net{this};
	ProcSysApi sys{this};

	ProcCoutBuf buf_out{this};
	ProcCerrBuf buf_err{this};
	std::ostream s_out{&buf_out};
	std::ostream s_err{&buf_err};
	std::optional<ProcessTask> task{nullptr};
	std::vector<std::string> args{};
	
	protected:
	
	WriterFn writer_{nullptr};
	ReaderFn reader_{nullptr};
	
	std::vector<FileDescriptor> descriptors_{};
	std::set<FileDescriptor> returned_descriptors_{};
	FileDescriptor descriptor_counter_{3};

	std::unordered_map<std::string, std::string> envvars_;

};