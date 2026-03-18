#include "os_basic.h"

#include "os.h"
#include "device.h"
#include "net_types.h"
#include "filesystem.h"
#include "os_input.h"
#include "os_fileio.h"
#include "os_shell.h"
#include "race_awaiter.h"
#include "proc_signal_awaiter.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <ranges>
#include <functional>
#include <system_error>

#include <signal.h>
#include <iso646.h>

EagerTask<int32_t> SshReader(Proc& proc, FileDescriptor fd)
{
	ProcNetApi& netapi = proc.net;

	while (netapi.socket_is_open(fd))
	{
		auto exp_read = co_await when_any(netapi.async_read_socket(fd), proc.wait(3));
		if (exp_read.index == 0)
		{
			if (auto res = std::get<1>(exp_read.value))
			{
				proc.write(*res);
			}
			else
			{
				proc.warnln("ssh: Reader error: {}. Exiting.", res.error().message());
				break;
			}
		}
		else if (exp_read.index == 1)
		{
			if (auto res = std::get<2>(exp_read.value))
			{
				proc.warnln("ssh: Reader abort: {}. Exiting.", res.message());
				break;
			}

			if (co_await netapi.async_socket_test_alive(fd, 3))
			{
				continue;
			}
			else
			{
				proc.warnln("ssh: Connection with server lost (3 attempts). Exiting.");
				break;
			}
		}
	}

	proc.signal(SIGTERM);

	co_return 0;
}

ProcessTask Programs::CmdSshClient(Proc& proc, std::vector<std::string> args)
{

	ProcNetApi& netapi = proc.net;
	
	CLI::App app{"Open a secure remote shell on another host."};
	app.allow_windows_style_options(false);

	struct SloginArgs
	{
		std::string addr;
		int32_t port{22};
		std::string username;
	} params{};

	app.add_option("ADDR,-a", params.addr, "The remote listening address")->required();
	app.add_option("-p", params.port, "The remote listening port")->capture_default_str();
	app.add_option("-u", params.username, "The username of the remote user");

	try
	{
		std::ranges::reverse(args);
		args.pop_back();
        app.parse(std::move(args));
    }
	catch(const CLI::ParseError& e)
	{
		int res = app.exit(e, proc.s_out, proc.s_err);
        co_return res;
    }

	auto exp_remote_addr = Address6::from_string(params.addr);
	if (not exp_remote_addr)
	{
		proc.errln("Invalid address '{}': {}.", params.addr, exp_remote_addr.error().what());
		co_return 1;
	}

	auto exp_sock = proc.net.create_socket();
	if (not exp_sock)
	{
		proc.errln("Failed to open socket: {}.", exp_sock.error().message());
		co_return 1;
	}

	FileDescriptor fd = *exp_sock;

	if (auto bind_err = netapi.bind_socket(fd, netapi.get_primary_ip(), 49152))
	{
		proc.errln("Failed to bind socket: {}.", bind_err.message());
		co_return 1;
	}

	proc.putln("Connecting to {}:{}...", params.addr, params.port);

	std::error_condition status = co_await netapi.async_connect_socket(fd, *exp_remote_addr, params.port);
	if (status.value() != 0)
	{
		proc.errln("Connection failed: {}.", status.message());
		co_return 1;
	}

	proc.putln("Connected.");

	SshReader(proc, fd);

	while (netapi.socket_is_open(fd))
	{
		auto exp_msg = co_await proc.read();

		if (not exp_msg)
		{
			proc.errln("ssh: Read failure: {}. Exiting.", exp_msg.error().message());
			co_return 2;
		}

		if (exp_msg->length() == 0)
		{
			continue;
		}

		co_await proc.net.async_write_socket(fd, *exp_msg);
	}
	
	co_return 0;
}

