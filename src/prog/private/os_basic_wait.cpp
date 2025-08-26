#include "os_basic.h"

#include "proc.h"

#include "CLI/CLI.hpp"

#include <string>

ProcessTask Programs::CmdWait(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	if (args.size() < 2)
	{
		proc.putln("Usage: wait [time (s)]");
		co_return 1;
	}

	float delay = static_cast<float>(std::atof(args[1].c_str()));
	co_await os.wait(delay);

	proc.putln("Waited {0} seconds.", delay);
	co_return 0;
}