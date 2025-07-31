#pragma once

#include "task.h"
#include "msg_queue.h"

#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <coroutine>
#include <optional>
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <memory>

class Proc;
class OS;

using ProcessTask = Task<int32_t, std::suspend_always>;
using process_args_t = std::function<ProcessTask(Proc&, std::vector<std::string>)>;
using input_await_t = std::function<void(const com::CommandQuery&)>;

class Proc
{
public:

	Proc(int32_t pid, OS* owner, std::ostream& out_stream)
		: pid(pid), owning_os(owner), out_stream(out_stream) { }

	Proc(int32_t pid, Proc* host)
		: pid(pid), host(host), owning_os(host->owning_os), out_stream(host->out_stream) { }

	Proc(int32_t pid, OS* owner)
		: Proc(pid, owner, std::cout) { }

	std::string get_name() const { return args.empty() ?  "?" : args[0]; }
	int32_t get_pid() const { return pid; }

	std::function<void(const std::string&)> writer = nullptr;

	/* Set this process running with a function and function arguments. 
	Optionally resume the provided coroutine immediately, if it is lazy. */
	void dispatch(process_args_t& program, std::vector<std::string> args, bool resume = true);

	/* Variant of 'dispatch' which awaits its own hosted process task. */
	EagerTask<int32_t> await_dispatch(process_args_t& program, std::vector<std::string> args);

	/* Write to the process 'standard output'. */
	template<typename ...Args>
	void put(std::format_string<Args...> fmt, Args&& ...args)
	{
		if (writer)
		{
			std::invoke(writer, std::format(fmt, std::forward<Args>(args)...));
			return;
		}

		if (host)
		{
			host->put(fmt, std::forward<Args>(args)...);
			return;
		}

		std::print(out_stream, fmt, std::forward<Args>(args)...);
	}

	/* Write to the process 'standard output', with  a \n at the end. */
	template<typename ...Args>
	void putln(std::format_string<Args...> fmt, Args&& ...args)
	{
		if (writer)
		{
			std::invoke(writer, std::format(fmt, std::forward<Args>(args)...).append("\n"));
			return;
		}

		if (host)
		{
			host->putln(fmt, std::forward<Args>(args)...);
			return;
		}

		std::println(out_stream, fmt, std::forward<Args>(args)...);
	}

	/* Execute a sub-process on this process. */
	EagerTask<int32_t> exec(std::string cmd);

	/* Execute a sub-process on this process. */
	EagerTask<int32_t> exec(com::CommandQuery query);

	int32_t pid{0};
	std::ostream& out_stream;
	OS* owning_os{nullptr};
	Proc* host{nullptr};
	std::optional<ProcessTask> task{nullptr};
	std::vector<std::string> args{};

};