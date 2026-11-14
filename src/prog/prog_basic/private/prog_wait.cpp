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

	float num = static_cast<float>(std::atof(args[1].c_str()));

	co_await proc.wait(num);

	co_return 0;
}