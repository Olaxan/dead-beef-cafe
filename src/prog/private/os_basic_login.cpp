#include "os_basic.h"
#include "os_input.h"
#include "os_fileio.h"

#include "os.h"
#include "sha256.h"
#include "users_mgr.h"

#include "CLI/CLI.hpp"

#include <ranges>
#include <charconv>
#include <unordered_set>


ProcessTask Programs::CmdLogin(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	UsersManager* users = os.get_users_manager();

	CmdInput::CmdReaderParams usr_params{};
	CmdInput::CmdReaderParams pwd_params{.password = true};

	proc.put(" | Username: ");
	auto exp_username = co_await CmdInput::read_cmd_utf8(proc, usr_params);
	if (not exp_username)
	{
		proc.errln("login: Read error: {}.", exp_username.error().message());
		co_return 1;
	}

	proc.put(" | Password: ");
	auto exp_password = co_await CmdInput::read_cmd_utf8(proc, usr_params);
	if (not exp_password)
	{
		proc.errln("login: Read error: {}.", exp_password.error().message());
		co_return 1;
	}

	users->prepare();

	if (std::optional<LoginPasswdData> passwd = users->authenticate(*exp_username, *exp_password); passwd.has_value())
	{
		/* Auth successful. */
		Proc* leader = proc.get_session_leader();
		leader->set_uid(passwd->uid);
		leader->set_gid(passwd->gid);

		proc.set_var("HOME", passwd->home_path);
		proc.set_var("SHELL", passwd->shell_path);
		proc.set_var("USER", passwd->username);
		proc.set_var("PWD", passwd->home_path);

		proc.putln("Welcome, {} ({}, {}).", *exp_username, passwd->uid, passwd->gid);
		
		co_return 0;
	}
	else
	{
		proc.warnln("Invalid credentials.");
		co_return 1;
	}
}