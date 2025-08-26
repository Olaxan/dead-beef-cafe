#include "os_basic.h"

#include "proc.h"
#include "filesystem.h"

#include "CLI/CLI.hpp"

#include <string>
#include <cstdlib>
#include <print>

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