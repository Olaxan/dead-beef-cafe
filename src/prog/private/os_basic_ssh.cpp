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
	NetManager* net = os.get_network_manager();
	Address6 local_ip = net->get_primary_ip();

	struct SSHNetSock
	{
		SocketDescriptor fd{0};
		NetManager* netmgr{nullptr};
	};

	auto exp_sock = net->create_socket();

	if (!exp_sock)
	{
		co_return 2;
	}

	if (!net->bind_socket(*exp_sock, {local_ip, 22}))
	{
		proc.errln("Failed to bind socket 22 for reading.");
		co_return 1;
	}

	SSHNetSock ssh_sock{ .fd = *exp_sock, .netmgr = net };

	/* --- WRITER FUNCTORS  ---
	Register writer functors in the process so that output
	is delivered in the form of command objects via the socket. */

	/* Writer for regular strings. */
	proc.add_writer<std::string>([ws = ssh_sock](const std::string& out_str)
	{
		com::CommandReply reply{};
		reply.set_reply(out_str);
		ws.netmgr->async_write_socket(ws.fd, out_str);
	});

	/* Writer for the common 'CommandReply' struct. */
	proc.add_writer<com::CommandReply>([ws = ssh_sock](const com::CommandReply& out_reply)
	{
		std::string coded_str;
		if (bool success = out_reply.SerializeToString(&coded_str))
			ws.netmgr->async_write_socket(ws.fd, coded_str);
	});



	/* --- READER FUNCTORS ---
	Add reader functors so that child processes can listen to our traffic. */

	/* Synchronous reader for common strings -- returns default if nothing in buffer. */
	proc.add_reader<std::string>([rs = ssh_sock]() -> std::any
	{
		if (std::optional<com::CommandQuery> opt = sock->read(); opt.has_value())
			return opt->command();
		
		return {};
	});

	/* Synchronous reader for 'CommandQuery' struct -- returns default if nothing in buffer. */
	proc.add_reader<com::CommandQuery>([ssh_sock]() -> std::any
	{
		if (std::optional<com::CommandQuery> opt = sock->read(); opt.has_value())
			return *opt;
		
		return {};
	});

	/* Async socket reader -> CommandQuery. */
	proc.add_reader<CmdSocketAwaiterServer>([ssh_sock]() -> std::any
	{
		return sock->async_read();
	});

	


	proc.putln("SecureShell version 5:");

	int32_t login_ret = co_await ShellUtils::Exec(proc, {"/sbin/login"});
	
	if (login_ret != 0)
	{
		proc.putln("Authentication failure.");
		co_return 1;
	}

	proc.putln("\nWelcome to " CSI_CODE(30;41) " DEAD::BEEF::CAFE " CSI_RESET ".\nPlease make sure you sign the g" CSI_CODE(4) "uest book" CSI_RESET "!\n");

	co_await ShellUtils::Exec(proc, {"/bin/shell"});

	co_return 0;
}