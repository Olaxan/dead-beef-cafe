#pragma once

#include "os.h"
#include "proc.h"

#include <coroutine>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <print>

namespace Programs
{
	ProcessTask CmdEcho(Proc& shell, std::vector<std::string> args)
	{
		//std::println("echo: {0}.", args);
		shell.write("echo: {0}", args);
		co_return 0;
	}

	ProcessTask CmdShutdown(Proc& shell, std::vector<std::string> args)
	{
		shell.owning_os->shutdown_os();
		co_return 0;
	}

	ProcessTask CmdCount(Proc& shell, std::vector<std::string> args)
	{
		if (args.size() < 2)
		{
			std::println("Usage: count [num]");
			co_return 1;
		}

		int32_t max = std::atoi(args[1].c_str());

		for (int32_t i = 0; i < max; ++i)
		{
			co_await shell.owning_os->wait(1.f);
			std::println("{0}...", i + 1);
		}

		co_return 0;
	}

	ProcessTask CmdProc(Proc& shell, std::vector<std::string> args)
	{
		shell.owning_os->list_processes();
		co_return 0;
	}

	ProcessTask CmdWait(Proc& shell, std::vector<std::string> args)
	{
		if (args.size() < 2)
		{
			std::println(shell.out_stream, "Usage: wait [time (s)]");
			co_return 1;
		}

		float delay = static_cast<float>(std::atof(args[1].c_str()));
		co_await shell.owning_os->wait(delay);

		std::println("Waited {0} seconds.", delay);
		co_return 0;
	}

	ProcessTask CmdShell(Proc& shell, std::vector<std::string> args)
	{
		while (true)
		{
			std::string in{};
			if (shell.in_stream >> in) //won't work, just PoC
			{
				int32_t ret = co_await shell.exec(in);
				shell.write("Process return with code {0}.", ret);
			}
			else
			{
				shell.write("{0}> ", shell.owning_os->get_hostname());
			}
		}
	}
}

class BasicOS : public OS
{
	void register_commands() override
	{
		commands_ = {
			{"echo", Programs::CmdEcho},
			{"count", Programs::CmdCount},
			{"shutdown", Programs::CmdShutdown},
			{"wait", Programs::CmdWait},
			{"proc", Programs::CmdProc},
			{"shell", Programs::CmdShell}
		};
	}

	process_args_t get_default_shell() const { return Programs::CmdShell; }
	
};