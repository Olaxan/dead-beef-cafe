#include "os_basic.h"
#include "os_input.h"

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
	SessionManager* sess = os.get_session_manager();

	if (fs == nullptr)
	{
		proc.errln("No filesystem!");
		co_return 2;
	}

	CmdInput::CmdReaderParams usr_params{};
	CmdInput::CmdReaderParams pwd_params{.password = true};

	proc.put("Username: ");
	std::string username = co_await CmdInput::read_cmd_utf8(proc, usr_params);

	proc.put("Password: ");
	std::string password = co_await CmdInput::read_cmd_utf8(proc, pwd_params);

	users->prepare();

	if (std::optional<LoginPasswdData> passwd = users->authenticate(username, password); passwd.has_value())
	{
		/* Auth successful. */
		proc.set_sid();
		proc.set_uid(passwd->uid);
		proc.set_gid(passwd->gid);
		//proc.add_groups(groups);

		proc.set_var("HOME", passwd->home_path);
		proc.set_var("SHELL", passwd->shell_path);
		proc.set_var("USER", passwd->username);
		proc.set_var("PWD", passwd->home_path);

		proc.putln("Welcome, {}.", username);
		
		co_return 0;
	}
	else
	{
		proc.warnln("Invalid credentials.");
		co_return 1;
	}
}

ProcessTask Programs::CmdUserAdd(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	CLI::App app{"Add users to the system. Requires root/su access."};
	app.allow_windows_style_options(false);
	
	struct UserAddArgs
	{
		std::string username{};
		std::string password{};
		std::string comment{};
		std::string home_path{};
		std::string exp_date{};
		int32_t inactive{0};
		std::vector<std::string> groups{};
		bool create_home{true};
	} params{};

	app.add_option("LOGIN", params.username, "Login name of the new user")->required();
	app.add_option("PASSWORD", params.password, "Password of the new user")->required();
	app.add_option("-d,--home", params.home_path, "Sets the user's login directory");
	app.add_option("-c,--comment", params.comment, "Any text string (sets the 'full name' GECOS field)");
	app.add_option("-e,--expiredate", params.exp_date, "The date on which the user account will be disabled (YYYY-MM-DD)");
	app.add_option("-f,--inactive", params.inactive, "The number of days after a password expires until the account is permanently suspended");
	app.add_option("-G,--groups", params.groups, "A list of supplementary groups which the user is also a member of");

	app.add_flag("-m,--create-home", params.create_home, "Create the user's home directory if it does not exist")->default_val(true);

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

	UsersManager* users = os.get_users_manager();
	assert(users);

	GecosData gecos{ .full_name = params.comment };

	bool success = users->add_user(params.username, params.password, {
		.create_home = params.create_home,
		.create_usergroup = true,
		.expiration_date = 0,
		.home_path = params.home_path,
		.gecos = std::move(gecos),
		.groups = std::move(params.groups)
	});

	if (success)
	{
		proc.putln("Successfully created user '{}'.", params.username);
		co_return 0;
	}
	else
	{
		proc.warnln("Failed to create user '{}'.", params.username);
		co_return 1;
	}

	co_return 1;
}

ProcessTask Programs::CmdGroupAdd(Proc& proc, std::vector<std::string> args)
{
	co_return 1;
}
