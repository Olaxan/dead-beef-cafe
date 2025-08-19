#include "os_basic.h"

#include "device.h"
#include "netw.h"
#include "filesystem.h"
#include "os_input.h"
#include "os_fileio.h"

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

ProcessTask Programs::CmdSudo(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();
	UsersManager* users = os.get_users_manager();

	if (fs == nullptr)
	{
		proc.errln("No filesystem!");
		co_return 2;
	}

	CmdInput::CmdReaderParams pwd_params{.password = true};

	Proc* session = proc.get_session_leader();
	int32_t uid = session->get_uid();
	std::optional<std::string_view> opt_user = users->get_username(uid);

	if (!opt_user)
	{
		proc.warnln("sudo: no session");
		co_return 1;
	}

	std::string username{*opt_user};
	constexpr int32_t sudo_max_tries{3};
	int32_t tries{0};

	while (true)
	{
		proc.put("[sudo] password for {}: ", username);
		std::string password = co_await CmdInput::read_cmd_utf8(proc, pwd_params);

		/* Success: we break out of the loop. */
		if (users->authenticate(username, password)) 
			break; 

		if (++tries < sudo_max_tries)
		{
			int32_t rem = sudo_max_tries - tries;
			proc.putln("Sorry, try again ({} attempt{} remaining)", rem, rem > 1 ? "s" : "");
		}
		else
		{
			proc.warnln("sudo: {} incorrect password attempts", sudo_max_tries);
			co_return 1;
		}
	}

	auto subargs = args 
	| std::views::drop(1) 
	| std::ranges::to<std::vector<std::string>>();

	if (subargs.empty())
		co_return 1;

	int32_t ret = co_await Programs::Exec(proc, std::move(subargs));

	co_return 0;
}