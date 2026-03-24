#include "os_basic.h"
#include "os_fileio.h"

#include "os.h"
#include "filesystem.h"
#include "sha256.h"
#include "users_mgr.h"

#include "CLI/CLI.hpp"

#include <ranges>
#include <charconv>
#include <unordered_set>


ProcessTask Programs::CmdUser(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	CLI::App app{"View information about a user."};
	app.allow_windows_style_options(false);
	app.require_subcommand(1);

	std::string user{};

	app.add_option("-u,--user", user, "View information about a specific user. By default the session UID is used");
	
	auto group = app.add_subcommand("groups", "List groups");
	
	group->callback([&]
	{

	});

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

	co_return 1;
}
