#include "os_basic.h"

#include "os.h"
#include "device.h"
#include "net_types.h"
#include "filesystem.h"
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

ProcessTask SSHSession(Proc& proc, std::vector<std::string> args)
{
	FileDescriptor fd = proc.get_var<FileDescriptor>("SSHCON");

	co_await proc.wait(1.f);

	proc.putln("SecureShell version 5:");

	int32_t login_ret = co_await ShellUtils::Exec(proc, {"/sbin/login"});
	
	if (login_ret != 0)
	{
		proc.putln("Authentication failure.");
		co_return 1;
	}

	proc.putln("\nWelcome to " CSI_CODE(30;41) " DEAD:BEEF:CAFE:: " CSI_RESET ".\nPlease make sure you sign the g" CSI_CODE(4) "uest book" CSI_RESET "!\n");

	int32_t ret = co_await ShellUtils::Exec(proc, {"/bin/shell"});
	proc.putln("Closing SSH session.");
	auto status = co_await proc.net.async_close_socket(fd);
	co_return ret;
}

ProcessTask Programs::CmdSshServer(Proc& proc, std::vector<std::string> args)
{

	OS& os = *proc.owning_os;
	ProcNetApi& netapi = proc.net;

	proc.putln("SSH service started.");

	auto exp_sock = netapi.create_socket();
	if (not exp_sock)
	{
		proc.errln("Failed to open socket: {}.", exp_sock.error().message());
		co_return 2;
	}

	FileDescriptor fd = *exp_sock;
	Address6 local_ip = netapi.get_primary_ip();

	if (auto bind_err = netapi.bind_socket(fd, {local_ip, 22}))
	{
		proc.errln("Failed to bind socket 22 for reading: {}.", bind_err.message());
		co_return 1;
	}

	if (netapi.listen(fd) != 0)
	{
		proc.errln("Failed to set socket 22 as a listening connection.");
	}

	while (netapi.socket_is_open(fd))
	{
		proc.putln("Waiting for connections ({}:{})...", local_ip, 22);
		DescriptorResult exp_con = co_await netapi.async_accept_socket(fd);

		if (not exp_con)
		{
			proc.errln("Accept failed: {}.", exp_con.error().message());
			continue;
		}

		FileDescriptor con = *exp_con;

		proc.putln("Connection established ({}).", con);
		proc.set_var("SSHCON", con);
	
		auto sess_reader = [con](const Proc& rproc) -> Task<ReadResult>
		{
			return rproc.net.async_read_socket(con);
		};
	
		auto sess_writer = [con](const Proc& wproc, const std::string& str)
		{
			com::CommandReply rep;
			rep.set_reply(str);
			
			std::string out_str;
			if (rep.SerializeToString(&out_str))
				wproc.net.async_write_socket(con, out_str);
		};
	
		OS::CreateProcessParams params
		{
			.writer = std::move(sess_writer),
			.reader = std::move(sess_reader),
			.fork_pid = proc.get_pid()
			//.leader_id = proc.get_pid()
			/* The reason we don't provide a session leader, 
			is that authentication would be given to the SSH host,
			as opposed to the SSH session... for now.*/
		};
	
		os.run_process(SSHSession, {std::format("ssh{}", con)}, std::move(params));
		netapi.close_socket(con); // As in, we release our claim on this fd.
	}

	co_return 0;
}