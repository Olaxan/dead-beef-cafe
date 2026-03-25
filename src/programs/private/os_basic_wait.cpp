#include "os_basic.h"

#include "proc.h"
#include "proc_signal_awaiter.h"

#include "CLI/CLI.hpp"

#include <string>

ProcessTask Programs::CmdWait(Proc& proc, std::vector<std::string> args)
{
	if (args.size() < 2)
	{
		proc.putln("Usage: wait [time (s)]");
		co_return 1;
	}

	float delay = static_cast<float>(std::atof(args[1].c_str()));

	auto res = co_await proc.wait(delay);
	if (res.value() > 0)
	{
		proc.errln("wait: Aborted: {}.", res.message());
		co_return 1;
	}

	proc.putln("Waited {0} seconds.", delay);
	co_return 0;
}