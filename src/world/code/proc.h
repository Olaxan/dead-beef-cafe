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

struct InputAwaiter
{
	explicit InputAwaiter(Proc* proc)
		: proc_(proc) { }

	bool await_ready();
	void await_suspend(std::coroutine_handle<> h);
	std::string await_resume() const;

private:

	std::optional<com::CommandQuery> next_{};
	Proc* proc_;

};

class Proc
{
public:

	Proc() = delete;

	Proc(int32_t pid, OS* owner, std::ostream& out_stream)
		: pid(pid), owning_os(owner), out_stream(out_stream) { }

	Proc(int32_t pid, OS* owner)
		: Proc(pid, owner, std::cout) { }

	std::string get_name() const { return args.empty() ?  "?" : args[0]; }
	int32_t get_pid() const { return pid; }

	std::function<void(const std::string&)> writer = [this](const std::string& out_str) { std::print(out_stream, out_str); };

	/* Set this process running with a function and function arguments. 
	Optionally resume the provided coroutine immediately, if it is lazy. */
	void dispatch(process_args_t& program, std::vector<std::string> args, bool resume = true)
	{
		this->args = std::move(args);
		task = std::move(std::invoke(program, *this, args));
		if (resume) task->handle.resume();
	}

	/* Variant of 'dispatch' which awaits its own hosted process task. */
	EagerTask<int32_t> await_dispatch(process_args_t& program, std::vector<std::string> args)
	{
		this->args = std::move(args);
		task = std::move(std::invoke(program, *this, args));
		task->handle.resume();
		co_return (co_await *task);
	}

	/* Write to the process 'standard output'. */
	template<typename ...Args>
	void put(const std::string& out_str, Args&& ...args)
	{
		std::invoke(writer, std::format(out_str, std::forward<Args>(args)));
	}

	/* Write to the process 'standard output', with  a \n at the end. */
	template<typename ...Args>
	void putln(const std::string& out_str, Args&& ...args)
	{
		std::invoke(writer, std::format(out_str, std::forward<Args>(args)).append('\n'));
	}

	/* Execute a sub-process on this process. */
	EagerTask<int32_t> exec(std::string cmd);

	/* Execute a sub-process on this process. */
	EagerTask<int32_t> exec(com::CommandQuery query);

	EagerTask<std::string> await_input();

	int32_t pid{0};
	std::ostream& out_stream;
	OS* owning_os{nullptr};
	std::optional<ProcessTask> task{nullptr};
	std::vector<std::string> args{};

};