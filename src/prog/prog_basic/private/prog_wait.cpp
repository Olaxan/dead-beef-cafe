#include "prog_basic.h"

#include "proc.h"
#include "proc_signal_awaiter.h"
#include "sync_awaiter_dynamic.h"

#include "CLI/CLI.hpp"

#include <string>

ProcessTask Programs::CmdWait(Proc& proc, std::vector<std::string> args)
{
	if (args.size() < 2)
	{
		proc.putln("Usage: wait [time (s)]");
		co_return 1;
	}

	std::size_t num = static_cast<std::size_t>(std::atof(args[1].c_str()));

	std::vector<Task<std::error_condition>> jobs;

	for (std::size_t i = 0; i < num; ++i)
	{
		jobs.emplace_back(proc.wait(1.f + static_cast<float>(i)));
	}

	auto res = co_await when_all_dynamic(std::move(jobs));

	std::size_t idx = 0;
	for (auto&& r : res)
	{
		proc.putln("job {}: {}.", idx, r.message());
		++idx;
	}

	co_return 0;
}