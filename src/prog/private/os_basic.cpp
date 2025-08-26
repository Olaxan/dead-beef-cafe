#include "os_basic.h"

#include "device.h"
#include "netw.h"
#include "proc.h"
#include "filesystem.h"

#include <string>
#include <cstdlib>
#include <print>


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
		{"/sbin/shutdown", Programs::CmdShutdown},
		{"/sbin/boot", Programs::CmdBoot },
		{"/sbin/login", Programs::CmdLogin},
		{"/sbin/useradd", Programs::CmdUserAdd},
		{"/sbin/groupadd", Programs::CmdGroupAdd},
		{"/bin/proc", Programs::CmdProc},
		{"/bin/wait", Programs::CmdWait},
		{"/bin/shell", Programs::CmdShell},
		{"/bin/ls", Programs::CmdList},
		{"/bin/mkdir", Programs::CmdMakeDir},
		{"/bin/touch", Programs::CmdMakeFile},
		{"/bin/open", Programs::CmdOpenFile},
		{"/bin/echo", Programs::CmdEcho},
		{"/bin/rm", Programs::CmdRemoveFile},
		{"/bin/cat", Programs::CmdCat},
		{"/bin/ping", Programs::CmdPing},
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

	fs->create_file("/etc/passwd", 
	{
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::Read | FilePermissionTriad::Write,
			.perm_group = FilePermissionTriad::Read,
			.perm_users = FilePermissionTriad::Read
		}
	});

	fs->create_file("/etc/shadow", 
	{
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::None,
			.perm_group = FilePermissionTriad::None,
			.perm_users = FilePermissionTriad::None
		}
	});

	fs->create_file("/etc/group", 
	{
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::Read | FilePermissionTriad::Write,
			.perm_group = FilePermissionTriad::Read,
			.perm_users = FilePermissionTriad::Read
		},
		.content = {
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
			"sudo:x:106:sysadmin"
		}
	});

	fs->create_file("/etc/sudoers", 
	{
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::Read,
			.perm_group = FilePermissionTriad::Read,
			.perm_users = FilePermissionTriad::None
		},
		.content = "fredr"
	});

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