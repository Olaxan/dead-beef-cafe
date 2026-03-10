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

ProcessTask SSHSession(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	NetManager* net = os.get_network_manager();

	co_await os.wait(1.f);

	proc.putln("SecureShell version 5:");

	int32_t login_ret = co_await ShellUtils::Exec(proc, {"/sbin/login"});
	
	if (login_ret != 0)
	{
		proc.putln("Authentication failure.");
		co_return 1;
	}

	proc.putln("\nWelcome to " CSI_CODE(30;41) " DEAD:BEEF:CAFE:: " CSI_RESET ".\nPlease make sure you sign the g" CSI_CODE(4) "uest book" CSI_RESET "!\n");

	co_return (co_await ShellUtils::Exec(proc, {"/bin/shell"}));
}

ProcessTask Programs::CmdSSH(Proc& proc, std::vector<std::string> args)
{

	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();
	UsersManager* users = os.get_users_manager();
	NetManager* net = os.get_network_manager();
	Address6 local_ip = net->get_primary_ip();

	proc.putln("SSH service started.");

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

	SocketDescriptor fd = *exp_sock;

	if (net->bind_socket(fd, {local_ip, 22}) != 0)
	{
		proc.errln("Failed to bind socket 22 for reading.");
		co_return 1;
	}

	SSHNetSock ssh_sock{ .fd = fd, .netmgr = net };

	if (net->listen(fd) != 0)
	{
		proc.errln("Failed to set socket 22 as a listening connection.");
	}

	while (net->socket_is_open(fd))
	{
		proc.putln("Waiting for connections ({}:{})...", local_ip, 22);
		SocketDescriptor con = co_await net->async_accept_socket(fd);
		proc.putln("Connection established ({}).", con);
	
		auto sess_reader = [net, con]() -> Task<std::string>
		{
			return net->async_read_socket(con);
		};
	
		auto sess_writer = [net, con](const std::string& str)
		{
			com::CommandReply rep;
			rep.set_reply(str);
			
			std::string out_str;
			if (rep.SerializeToString(&out_str))
				net->async_write_socket(con, out_str);
		};
	
		OS::CreateProcessParams params
		{
			.writer = std::move(sess_writer),
			.reader = std::move(sess_reader),
			//.leader_id = proc.get_pid()
			/* The reason we don't provide a session leader, 
			is that authentication would be given to the SSH host,
			as opposed to the SSH session... for now.*/
		};
	
		os.run_process(SSHSession, {std::format("ssh{}", con)}, std::move(params));	
	}

	co_return 0;
}