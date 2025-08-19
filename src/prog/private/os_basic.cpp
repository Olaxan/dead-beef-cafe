#include "os_basic.h"

#include "device.h"
#include "netw.h"
#include "proc.h"
#include "filesystem.h"

#include "CLI/CLI.hpp"

#include <string>
#include <cstdlib>
#include <print>

ProcessTask Programs::CmdEcho(Proc& proc, std::vector<std::string> args)
{
	proc.putln("echo: {0}", args);
	co_return 0;
}

ProcessTask Programs::CmdCount(Proc& proc, std::vector<std::string> args)
{
	CLI::App app{"A program that counts!"};

	float delay = 1.f;
	int32_t count = 5;
	app.add_option("COUNT", count, "The number to count to")->default_val(5);
	app.add_option("-d", delay, "The time to wait per count");

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

	for (int32_t i = 0; i < count; ++i)
	{
		co_await proc.owning_os->wait(delay);
		proc.putln("{0}...", i + 1);
	}

	co_return 0;
}

ProcessTask Programs::CmdProc(Proc& proc, std::vector<std::string> args)
{
	proc.putln("Processes on {0}:", proc.owning_os->get_hostname());
	proc.owning_os->get_processes([&proc](const Proc& process)
	{
		proc.putln("{0}: '{1}'", process.get_pid(), process.get_name());
	});
	co_return 0;
}

ProcessTask Programs::CmdWait(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	if (args.size() < 2)
	{
		proc.putln("Usage: wait [time (s)]");
		co_return 1;
	}

	float delay = static_cast<float>(std::atof(args[1].c_str()));
	co_await os.wait(delay);

	proc.putln("Waited {0} seconds.", delay);
	co_return 0;
}

BasicOS::BasicOS(Host& owner) : OS(owner)
{
	register_devices();

	FileSystem* fs = get_filesystem();
	if (fs == nullptr)
		return;

	fs->create_directory("/dev", {
		.recurse = true,
		.meta = {
			.perm_owner	= FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}
	});

	fs->create_directory("/bin", {
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}
	});

	fs->create_directory("/etc", {
		.recurse = true,
		.meta = {
			.perm_owner	= FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}
	});

	fs->create_directory("/home", {
		.recurse = true,
		.meta = {
			.perm_owner	= FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}
	});

	fs->create_directory("/lib", {
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}
	});

	fs->create_directory("/sbin", {
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}
	});

	fs->create_directory("/tmp", {
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::All,
			.perm_users = FilePermissionTriad::All
		}	
	});

	fs->create_directory("/var/log", {
		.recurse = true,
		.meta = {
			.perm_owner	= FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}
	});

	fs->create_directory("/var/lock", {
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::All,
			.perm_users = FilePermissionTriad::All
		}		
	});

	fs->create_directory("/var/tmp", {
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::All,
			.perm_users = FilePermissionTriad::All
		}
	});

	fs->create_directory("/usr/bin", {
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}	
	});

	fs->create_directory("/usr/lib", {
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}
	});

	fs->create_directory("/usr/local", {
		.recurse = true,
		.meta = {
			.perm_owner	= FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}	
	});

	fs->create_directory("/usr/share", {
		.recurse = true,
		.meta = {
			.perm_owner	= FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		}		
	});

	std::vector<std::pair<std::string, ProcessFn>> os_progs = 
	{
		{"/bin/proc", Programs::CmdProc},
		{"/bin/wait", Programs::CmdWait},
		{"/bin/shell", Programs::CmdShell},
		{"/sbin/shutdown", Programs::CmdShutdown},
		{"/sbin/boot", Programs::CmdBoot },
		{"/sbin/login", Programs::CmdLogin},
		{"/sbin/useradd", Programs::CmdUserAdd},
		{"/sbin/groupadd", Programs::CmdGroupAdd},
		{"/bin/ls", Programs::CmdList},
		{"/bin/mkdir", Programs::CmdMakeDir},
		{"/bin/touch", Programs::CmdMakeFile},
		{"/bin/open", Programs::CmdOpenFile},
		{"/bin/echo", Programs::CmdEcho},
		{"/bin/rm", Programs::CmdRemoveFile},
		{"/bin/cat", Programs::CmdCat},
		{"/usr/bin/count", Programs::CmdCount},
		{"/usr/bin/snake", Programs::CmdSnake},
		{"/usr/bin/dogs", Programs::CmdDogs},
		{"/usr/bin/edit", Programs::CmdEdit},
		{"/lib/modules/kernel/drivers/cpu", Programs::InitCpu},
		{"/lib/modules/kernel/drivers/net", Programs::InitNet},
		{"/lib/modules/kernel/drivers/disk", Programs::InitDisk}
	};

	for (auto& fn : os_progs)
	{
		fs->create_file(fn.first, 
		{
			.recurse = true,
			.meta = {
				.perm_owner = FilePermissionTriad::All,
				.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
				.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
			},
			.executable = std::forward<ProcessFn>(fn.second)
		});
	}

	fs->create_file("/bin/sudo", 
	{
		.meta = {
			.perm_owner = FilePermissionTriad::Execute,
			.perm_group = FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Execute,
			.extra = ExtraFileFlags::SetUid
		},
		.executable = std::forward<ProcessFn>(Programs::CmdSudo)
	});

	FileSystem::CreateFileParams passwd_params = {
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::Read | FilePermissionTriad::Write,
			.perm_group = FilePermissionTriad::Read,
			.perm_users = FilePermissionTriad::Read
		}
	};

	fs->create_file("/etc/passwd", passwd_params);

	FileSystem::CreateFileParams shadow_params = {
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::None,
			.perm_group = FilePermissionTriad::None,
			.perm_users = FilePermissionTriad::None
		}
	};

	fs->create_file("/etc/shadow", shadow_params);

	if (auto [fid, ptr, err] = fs->create_file("/etc/group", passwd_params); err == FileSystemError::Success)
	{
		ptr->write(
			"root:x:0:\n"
			"daemon:x:1:\n"
			"bin:x:2:\n"
			"sys:x:3:\n"
			"adm:x:4:sysadmin\n"
			"tty:x:5:\n"
			"disk:x:6:\n"
			"lp:x:7:\n"
			"mem:x:8:\n"
			"kmem:x:9:\n"
			"wheel:x:10:sysadmin\n"
			"mail:x:12:postfix\n"
			"man:x:15:\n"
			"users:x:100:\n"
			"nogroup:x:65534:\n"
			"ssh:x:105:\n"
			"sudo:x:106:fredr,sysadmin");
	}

	users_.prepare();

	users_.add_user("root", "gong", {
		.create_home = false,
		.uid = 0,
		.gid = 0,
		.shell_path = "/bin/shell",
	}, false);

	users_.add_user("fredr", "hello", {
		.uid = 1000,
		.gid = 1000,
		.shell_path = "/bin/shell",
		.home_path = "/home/fredr"
	}, false);

	users_.commit();
}