#include "proc.h"
#include "os.h"
#include "task.h"

#include <coroutine>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

void Proc::dispatch(ProcessFn &program, std::vector<std::string> args, bool resume)
{
	this->args = std::move(args);
	task = std::move(std::invoke(program, *this, args));
	if (resume) task->handle.resume();
}

EagerTask<int32_t> Proc::await_dispatch(ProcessFn &program, std::vector<std::string> args)
{
	this->args = std::move(args);
	task = std::move(std::invoke(program, *this, this->args));
	task->handle.resume();
	co_return (co_await *task);
}

EagerTask<int32_t> Proc::exec(std::string cmd)
{
	std::string temp{};
	std::vector<std::string> args{};
	
	std::stringstream ss(cmd);

	while (ss >> std::quoted(temp))
	{
		args.push_back(std::move(temp));
	}

    if (args.empty())
        co_return 1;

    const std::string& word = *std::begin(args);

	if (auto [it, success] = owning_os->get_program(word); success)
	{
		auto& prog = it->second;
		int32_t ret = co_await owning_os->create_process(prog, std::move(args), this);
		co_return ret;
	}

	warnln("'{0}': No such file or directory.", word);
	co_return 1;
}

/* Await input, awaiters */

EagerTask<int32_t> Proc::exec(com::CommandQuery query)
{
	co_return (co_await exec(query.command()));
}