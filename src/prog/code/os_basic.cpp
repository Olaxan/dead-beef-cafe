#include "os_basic.h"

#include "device.h"
#include "netw.h"

#include <string>
#include <cstdlib>
#include <print>

ProcessTask Programs::CmdEcho(Proc &proc, std::vector<std::string> args)
{
	proc.putln("echo: {0}", args);
	co_return 0;
}

ProcessTask Programs::CmdCount(Proc &proc, std::vector<std::string> args)
{
	if (args.size() < 2)
	{
		proc.putln("Usage: count [num]");
		co_return 1;
	}

	int32_t max = std::atoi(args[1].c_str());

	for (int32_t i = 0; i < max; ++i)
	{
		co_await proc.owning_os->wait(1.f);
		proc.putln("{0}...", i + 1);
	}

	co_return 0;
}

ProcessTask Programs::CmdProc(Proc &proc, std::vector<std::string> args)
{
	proc.putln("Processes on {0}:", proc.owning_os->get_hostname());
	proc.owning_os->get_processes([&proc](const Proc& process)
	{
		proc.putln("{0}: '{1}'", process.get_pid(), process.get_name());
	});
	co_return 0;
}

ProcessTask Programs::CmdWait(Proc &proc, std::vector<std::string> args)
{
	if (args.size() < 2)
	{
		proc.putln("Usage: wait [time (s)]");
		co_return 1;
	}

	float delay = static_cast<float>(std::atof(args[1].c_str()));
	co_await proc.owning_os->wait(delay);

	proc.putln("Waited {0} seconds.", delay);
	co_return 0;
}