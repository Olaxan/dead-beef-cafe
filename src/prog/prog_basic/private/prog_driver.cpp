#include "prog_basic.h"

#include "device.h"
#include "net_types.h"

#include "cpu.h"
#include "disk.h"
#include "nic.h"

#include <string>
#include <vector>
#include <print>

EagerTask<int32_t> loading_bar(Proc& proc, std::size_t length, std::string result)
{
	proc.put("[");
	for (std::size_t i = 0; i < length; ++i)
	{
		proc.put("I");
		co_await proc.owning_os->wait(0.02f);
	}
	proc.putln("] [{}]", result);
	co_return 0;
}

ProcessTask Programs::InitCpu(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	co_return 0;
}

ProcessTask Programs::InitNet(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	co_return 0;
}

ProcessTask Programs::InitDisk(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	co_return 0;
}
