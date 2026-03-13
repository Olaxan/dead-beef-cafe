#include "os_basic.h"

#include "proc.h"
#include "filesystem.h"

#include "CLI/CLI.hpp"

#include <string>
#include <cstdlib>
#include <print>

ProcessTask Programs::CmdKill(Proc& proc, std::vector<std::string> args)
{

	OS& os = *proc.owning_os;

	CLI::App app{"Send signals to running processes based on their process id."};
	app.allow_windows_style_options(false);

	struct KillArgs
	{
		int32_t pid;
	} params{};

	app.add_option("PID", params.pid, "Process id of the program to terminate");

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

	os.kill_process(params.pid);

	co_return 1;
}