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

EagerTask<int32_t> create_ssh_worker(SocketDescriptor fd)
{
	co_return 0;
}

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

	SocketDescriptor fd = *exp_sock;

	if (!net->bind_socket(fd, {local_ip, 22}))
	{
		proc.errln("Failed to bind socket 22 for reading.");
		co_return 1;
	}

	SSHNetSock ssh_sock{ .fd = fd, .netmgr = net };

	if (!net->listen(fd))
	{
		proc.errln("Failed to set socket 22 as a listening connection.");
	}

	// while (net->socket_is_open(fd))
	// {
	// 	if (auto&& [ec, peer_fd] = co_await net->accept(fd); net->socket_is_open(peer_fd))
	// 	{
	// 		create_ssh_worker(peer_fd);
	// 	}
	// }

	/* --- WRITER FUNCTORS  ---
	Register writer functors in the process so that output
	is delivered in the form of command objects via the socket. */

	/* Writer for regular strings. */
	proc.set_writer([ws = ssh_sock](const std::string& out_str)
	{
		ws.netmgr->async_write_socket(ws.fd, out_str);
	});

	/* --- READER FUNCTORS ---
	Add reader functors so that child processes can listen to our traffic. */

	/* Asynchronous reader for common strings. */
	proc.set_reader([rs = ssh_sock]() -> ProcessReadAwaiter
	{
		return rs.netmgr->async_read_socket(rs.fd);
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