#include "prog_basic.h"

#include "device.h"
#include "net_types.h"
#include "filesystem.h"

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


ProcessTask Programs::InitProg(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	co_return 0;
}

ProcessTask Programs::CmdBoot(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No filesystem!");
		co_return 1;
	}

	if (DeviceState state = os.get_state(); state != DeviceState::PoweredOff)
	{
		proc.warnln("Invalid boot state '{}'.", DeviceUtils::get_state_name(state));
		co_return 1;
	}

	proc.putln("Sending wake-on-LAN request to {0}...", os.get_hostname());
	
	co_await os.wait(2.f);

	os.set_state(DeviceState::PoweredOn);

	co_return 0;
}

ProcessTask Programs::CmdShutdown(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	proc.putln("Shutting down {0}...", os.get_hostname());
	os.set_state(DeviceState::PoweredOff);

	co_return 0;
}