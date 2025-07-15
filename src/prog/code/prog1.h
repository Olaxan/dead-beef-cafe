#pragma once

#include "os.h"
#include "task.h"

#include <coroutine>
#include <string>
#include <vector>
#include <iostream>
#include <print>

namespace Programs
{
	ProcessTask CmdEcho(OS* env, std::vector<std::string> args)
	{
		std::println("echo: {0}.", args);
		co_return;
	}

	ProcessTask CmdShutdown(OS* env, std::vector<std::string> args)
	{
		std::println("Shutting down {0}...", env->get_hostname());
		env->shutdown_os();
		co_return;
	}

	ProcessTask CmdCount(OS* env, std::vector<std::string> args)
	{
		std::println("Counting to ten...");

		for (int32_t i = 0; i < 10; ++i)
		{
			co_await env->wait(1.f);
			std::println("{0}...", i);
		}
	}
}