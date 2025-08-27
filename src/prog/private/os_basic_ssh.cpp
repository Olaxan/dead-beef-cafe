#include "os_basic.h"

#include "os.h"
#include "device.h"
#include "netw.h"
#include "filesystem.h"
#include "os_input.h"
#include "os_fileio.h"
#include "os_shell.h"

#include "CLI/CLI.hpp"

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/usearch.h>
#include <unicode/ustring.h>
#include <unicode/ustream.h>
#include <unicode/brkiter.h>

#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <ranges>
#include <functional>

ProcessTask Programs::CmdSSH(Proc& proc, std::vector<std::string> args)
{

	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();
	UsersManager* users = os.get_users_manager();

	auto sock = os.create_socket<CmdSocketServer>();
	if (!os.bind_socket(sock, 22))
	{
		proc.errln("Failed to bind socket 22 for reading.");
		co_return 1;
	}

	/* Register writer functors in the process so that output
	is delivered in the form of command objects via the socket. */
	proc.add_writer<std::string>([sock](const std::string& out_str)
	{
		com::CommandReply reply{};
		reply.set_reply(out_str);
		sock->write(std::move(reply));
	});

	/* Add writer for CommandReply so we can send such directly. */
	proc.add_writer<com::CommandReply>([sock](const com::CommandReply& out_reply)
	{
		sock->write(out_reply);
	});

	/* Add reader functors so that child processes can listen to our traffic. */
	proc.add_reader<std::string>([sock]() -> std::any
	{
		if (std::optional<com::CommandQuery> opt = sock->read(); opt.has_value())
			return opt->command();
		
		return {};
	});

	/* Async socket reader -> CommandQuery. */
	proc.add_reader<CmdSocketAwaiterServer>([sock]() -> std::any
	{
		return sock->async_read();
	});

	/* Synchronous socket reader -> CommandQuery*/
	proc.add_reader<com::CommandQuery>([sock]() -> std::any
	{
		if (std::optional<com::CommandQuery> opt = sock->read(); opt.has_value())
			return *opt;
		
		return {};
	});

	proc.putln("SecureShell version 5:");

	//co_await ShellUtils::Exec(proc, {"/sbin/login"});

	proc.putln("\nWelcome to " CSI_CODE(30;41) " DEAD::BEEF::CAFE " CSI_RESET ".\nPlease make sure you sign the g" CSI_CODE(4) "uest book" CSI_RESET "!\n");

	co_await ShellUtils::Exec(proc, {"/bin/shell"});

	co_return 0;
}