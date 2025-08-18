#pragma once

#include "task.h"
//#include "msg_queue.h"
#include "term_utils.h"
#include "session.h"

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
#include <any>
#include <typeindex>
#include <sstream>
#include <ostream>

class Proc;
class OS;

using ProcessTask = Task<int32_t, std::suspend_always>;
using ProcessFn = std::function<ProcessTask(Proc&, std::vector<std::string>)>;
using WriterFn = std::function<void(const std::string&)>;
using ReaderFn = std::function<std::any(void)>;

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

	Proc(int32_t pid, OS* owner, std::ostream& out_stream = std::cout)
		: owning_os(owner), pid(pid), out_stream(out_stream)
	{ }

	Proc(int32_t pid, Proc* fork)
		: host(fork), owning_os(fork->owning_os), pid(pid), sid(fork->sid), out_stream(fork->out_stream)
	{ }

	~Proc()
	{
		s_err << std::flush;
		s_out << std::flush;
	}

	std::string_view get_name() const { return args.empty() ?  "?" : std::string_view(args[0]); }
	int32_t get_pid() const { return pid; }


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
	std::string_view get_var(std::string key, EnvVarAccessMode mode = EnvVarAccessMode::Inherit) const
	{
		if (auto it = envvars_.find(key); it != envvars_.end())
			return it->second;

		if (host && mode == EnvVarAccessMode::Inherit)
			return host->get_var(key, mode);
		
		return {};
	}

	std::string_view get_var_or(std::string key, std::string_view or_value, EnvVarAccessMode mode = EnvVarAccessMode::Inherit) const
	{
		std::string_view get = get_var(key, mode);
		return !get.empty() ? get : or_value;
	}


	/* ---- FUNCTIONS THAT RELATE TO GETTING INPUT FROM THE USER --- */

	/* Add a reader of a certain type, which can be used in read function calls. */
	template <typename T>
    void add_reader(std::function<std::any()> reader) 
	{
        readers_[std::type_index(typeid(T))] = [reader]() 
		{
            return reader();
        };
    }

	/* Try to read something from a reader function, if it has been provided. 
	The data type must be specified and match, or an exception will be raised. */
	template<typename T> 
	std::optional<T> read(EnvVarAccessMode mode = EnvVarAccessMode::Inherit)
	{
		if (auto it = readers_.find(std::type_index(typeid(T))); it != readers_.end())
		{
			try
			{
				if (std::any&& get = it->second(); get.has_value())
				{
					return std::any_cast<T>(get);
				}
			}
			catch (const std::bad_any_cast& e)
			{
				std::cerr << e.what() << std::endl;
			}
		}

		if (host && mode == EnvVarAccessMode::Inherit)
			return host->read<T>(mode);

		return std::nullopt;
	}

	/* ---- FUNCTIONS THAT RELATE TO PUTTING THINGS ON THE TERMINAL --- */

	/* Register a writer of a certain type, to be used in put/write function calls instead of standard out. */
	template <typename T>
    void add_writer(std::function<void(const T&)> writer) 
	{
        writers_[std::type_index(typeid(T))] = [writer](const void* msg) 
		{
            writer(*static_cast<const T*>(msg));
        };
    }

	/* Templated function of put/warn/err which allows to write any kind of data to writer map. */
	template <typename T>
    bool write(const T& msg) 
	{
        if (auto it = writers_.find(std::type_index(typeid(T))); it != writers_.end())
		{
            it->second(&msg);
			return true;
		}

		if (host)
			return host->write<T>(msg);

		return false;
    }

	/* Overload to ensure string literals work. */
    bool write(const char* msg) 
	{
        return write(std::string(msg));
    }

	/* Write to the process 'standard output'. */
	template<typename ...Args>
	void put(std::format_string<Args...> fmt, Args&& ...args)
	{
		if (write(std::format(fmt, std::forward<Args>(args)...)))
			return;

		std::print(out_stream, fmt, std::forward<Args>(args)...);
	}

	/* Write to the process 'standard output', with  a \n at the end. */
	template<typename ...Args>
	void putln(std::format_string<Args...> fmt, Args&& ...args)
	{
		if (write(std::format(fmt, std::forward<Args>(args)...).append("\n")))
			return;

		std::println(out_stream, fmt, std::forward<Args>(args)...);
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


	/* --- FUNCTIONS THAT RELATE TO SESSION --- */

	int32_t set_sid();
	int32_t get_uid() const;
	int32_t get_gid() const;

public:

	Proc* host{nullptr};
	OS* owning_os{nullptr};
	int32_t pid{0};
	int32_t sid{0};
	std::ostream& out_stream;
	ProcCoutBuf buf_out{this};
	ProcCerrBuf buf_err{this};
	std::ostream s_out{&buf_out};
	std::ostream s_err{&buf_err};
	std::optional<ProcessTask> task{nullptr};
	std::vector<std::string> args{};
	std::any data_{};
	std::unordered_map<std::type_index, std::function<void(const void*)>> writers_;
	std::unordered_map<std::type_index, std::function<std::any()>> readers_;

private:

	std::unordered_map<std::string, std::string> envvars_;

};