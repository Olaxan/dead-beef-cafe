#include "os_basic.h"
#include "os_input.h"
#include "os_fileio.h"

#include "os.h"
#include "filesystem.h"
#include "sha256.h"
#include "users_mgr.h"

#include "CLI/CLI.hpp"

#include <ranges>
#include <charconv>
#include <unordered_set>


ProcessTask Programs::CmdLogin(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();
	UsersManager* users = os.get_users_manager();

	if (fs == nullptr)
	{
		proc.errln("No filesystem!");
		co_return 2;
	}

	CmdInput::CmdReaderParams usr_params{};
	CmdInput::CmdReaderParams pwd_params{.password = true};

	proc.put(" | Username: ");
	std::string username = co_await CmdInput::read_cmd_utf8(proc, usr_params);

	proc.put(" | Password: ");
	std::string password = co_await CmdInput::read_cmd_utf8(proc, pwd_params);

	users->prepare();

	if (std::optional<LoginPasswdData> passwd = users->authenticate(username, password); passwd.has_value())
	{
		/* Auth successful. */
		Proc* leader = proc.get_session_leader();
		leader->set_uid(passwd->uid);
		leader->set_gid(passwd->gid);

		proc.set_var("HOME", passwd->home_path);
		proc.set_var("SHELL", passwd->shell_path);
		proc.set_var("USER", passwd->username);
		proc.set_var("PWD", passwd->home_path);

		proc.putln("Welcome, {} ({}, {}).", username, passwd->uid, passwd->gid);
		
		co_return 0;
	}
	else
	{
		proc.warnln("Invalid credentials.");
		co_return 1;
	}
}