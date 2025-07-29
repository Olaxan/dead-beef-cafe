#pragma once

#include "os.h"
#include "proc.h"
#include "netw.h"

#include <coroutine>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <print>

using CmdSocketServer = Socket<com::CommandQuery, com::CommandReply>;
using CmdSocketClient = Socket<com::CommandReply, com::CommandQuery>;

namespace Programs
{
	ProcessTask CmdEcho(Proc& shell, std::vector<std::string> args)
	{
		//std::println("echo: {0}.", args);
		shell.putln("echo: {0}", args);
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
		OS& our_os = *shell.owning_os;
		auto sock = our_os.create_socket<CmdSocketServer>();
		our_os.bind_socket(sock, 22);

		/* Replace the writer functor in the process so that output
		is delivered in the form of command objects via the socket. */
		shell.writer = [sock](const std::string& out_str)
		{
			com::CommandReply reply{};
			reply.set_reply(out_str);
			sock->write_one(std::move(reply));
		};

		while (true)
		{
			shell.put("{0}> ", shell.owning_os->get_hostname());
			com::CommandQuery input = co_await sock->read_one();
			int32_t ret = co_await shell.exec(input.command());
			shell.putln("Process returned with code {0}.", ret);
		}

		co_return 0;
	}
}

class BasicOS : public OS
{
public:

	BasicOS() = delete;
	
	BasicOS(Host& owner) : OS(owner) { };

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