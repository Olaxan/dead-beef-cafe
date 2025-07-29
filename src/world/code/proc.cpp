#include "proc.h"
#include "os.h"
#include "task.h"

#include <coroutine>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

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
		int32_t ret = co_await owning_os->create_process(prog, std::move(args), out_stream);
	}
}

/* Await input, awaiters */

EagerTask<int32_t> Proc::exec(com::CommandQuery query)
{
	co_return (co_await exec(query.command()));
}