#pragma once

#include "os.h"
#include "task.h"

#include <coroutine>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <print>

namespace Programs
{
	Task<int32_t> CmdEcho(OS* env, std::vector<std::string> args)
	{
		std::println("echo: {0}.", args);
		co_return 0;
	}

	Task<int32_t> CmdShutdown(OS* env, std::vector<std::string> args)
	{
		std::println("Shutting down {0}...", env->get_hostname());
		env->shutdown_os();
		co_return 0;
	}

	Task<int32_t> CmdCount(OS* env, std::vector<std::string> args)
	{
		if (args.size() < 2)
		{
			std::println("Usage: count [num]");
			co_return 1;
		}

		int32_t max = std::atoi(args[1].c_str());

		for (int32_t i = 0; i < max; ++i)
		{
			co_await env->wait(1.f);
			std::println("{0}...", i + 1);
		}

		co_return 0;
	}

	Task<int32_t> CmdProc(OS* env, std::vector<std::string> args)
	{
		env->list_processes();
		co_return 0;
	}

	Task<int32_t> CmdWait(OS* env, std::vector<std::string> args)
	{
		if (args.size() < 2)
		{
			std::println("Usage: wait [time (s)]");
			co_return 1;
		}

		float delay = static_cast<float>(std::atof(args[1].c_str()));
		co_await env->wait(delay);

		std::println("Waited {0} seconds.", delay);
		co_return 0;
	}
}