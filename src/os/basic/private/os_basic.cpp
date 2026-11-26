#include "os_basic.h"

#include "host.h"
#include "device.h"
#include "net_types.h"
#include "proc.h"
#include "filesystem.h"

#include "prog_basic.h"

#include <string>
#include <cstdlib>
#include <print>


BasicOS::BasicOS() : OS()
{
	reinstall_os();
}

BasicOS::~BasicOS()
{

}

void BasicOS::start_os()
{
	state_ = DeviceState::PoweredOn;
	run_process(Programs::SrvNetTx, {"nettx"}, {});
	run_process(Programs::SrvNetRx, {"netrx"}, {});
	run_process(Programs::CmdSshServer, {"ssh"}, {});
}

void BasicOS::reinstall_os()
{
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