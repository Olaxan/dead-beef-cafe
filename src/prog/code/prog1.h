#pragma once

#include "os.h"
#include "process.h"

#include <coroutine>
#include <string>
#include <vector>
#include <iostream>

namespace Programs
{
	ProcessTask CmdEcho(OS& env, std::vector<std::string> args)
	{
		for (auto& arg : args)
			std::cout << arg << std::endl;

		std::cout << std::endl;

		co_return;
	}

	ProcessTask CmdShutdown(OS& env, std::vector<std::string> args)
	{
		env.shutdown_os();
		co_return;
	}

	ProcessTask CmdCount(OS& env, std::vector<std::string> args)
	{
		for (int32_t i = 0; i < 10; ++i)
		{
			co_await env.wait(1.5f);
			std::cout << i;
		}
	}
}