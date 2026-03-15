#include "os_basic.h"

#include "os.h"
#include "device.h"
#include "net_types.h"
#include "filesystem.h"
#include "os_input.h"
#include "os_fileio.h"
#include "os_shell.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <ranges>
#include <functional>
#include <system_error>

#include <iso646.h>

ProcessTask Programs::CmdSshClient(Proc& proc, std::vector<std::string> args)
{
	
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

	proc.net.bind_socket(fd, proc.net.get_primary_ip(), 49152);
	std::error_condition status = co_await proc.net.async_connect_socket(fd, *exp_remote_addr, params.port);
	if (status.value() != 0)
	{
		proc.errln("Connection failed: {}.", status.message());
		co_return 1;
	}

	while (proc.net.socket_is_open(fd))
	{
		auto exp_msg = co_await proc.read();
		if (not exp_msg)
		{
			proc.errln("Read failure: {}. Exiting.", exp_msg.error().message());
			co_return 2;
		}

		co_await proc.net.async_write_socket(fd, *exp_msg);
	}
	
	co_return 0;
}

