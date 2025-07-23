#pragma once

#include "task.h"

#include <coroutine>
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <memory>

class Proc;

using ProcessTask = Task<int32_t, std::suspend_always>;
using process_args_t = std::function<ProcessTask(Proc&, std::vector<std::string>)>;

class Proc
{
public:

	Proc() = delete;

	Proc(int32_t pid, OS* owner, std::istream& in_stream, std::ostream& out_stream)
		: pid(pid), in_stream(in_stream), out_stream(out_stream), owning_os(owner) { }

	Proc(int32_t pid, OS* owner)
		: Proc(pid, owner, std::cin, std::cout) { }

	int32_t pid{0};
	std::istream& in_stream;
	std::ostream& out_stream;
	OS* owning_os{nullptr};
	std::optional<ProcessTask> task{nullptr};
	std::vector<std::string> args{};

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

	/* Read from the process 'standard input'. */
	std::string read()
	{
		std::string temp{};
		std::getline(in_stream, temp);
		return temp; //RVO?
	}

	/* Write to the process 'standard output'. */
	template<typename ...Args>
	void write(const std::string& out_str, Args&& ...args)
	{
		std::println(out_stream, out_str, std::forward<Args>(args));
	}

	/* Execute a sub-process on this process. */
	EagerTask<int32_t> exec(std::string cmd);

};