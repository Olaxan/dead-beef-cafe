#include "os_basic.h"

#include "device.h"
#include "netw.h"
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

	std::size_t num_dev = os.register_devices();
	std::println("Registered {} PCIE devices.", num_dev);

	for (auto& [uuid, dev] : os.get_devices())
	{
		proc.putln("[INIT] -------- uuid={} '{}' -------- ", uuid, dev->get_device_id());
		co_await os.wait(0.25f);

		std::string driver = dev->get_driver_id();
		proc.putln("[INIT] found device driver '{}'", driver);
		co_await os.wait(0.25f);

		int32_t ret = co_await std::invoke([&proc, &os, &fs](const std::string& driver_name) -> EagerTask<int32_t>
		{
			FilePath driver_path(std::format("/lib/modules/kernel/drivers/{}", driver_name));

			if (auto [fid, ptr, err] = fs->open(driver_path); err == FileSystemError::Success)
			{
				// if (auto& prog = ptr->get_executable())
				// {
				// 	int32_t ret = co_await os.create_process(prog, {}, &proc);
				// 	co_return ret;
				// }
				// else
				// {
				// 	proc.errln("Access violation.");
				// 	co_return 1;
				// }
				co_return 0;
			}
			else
			{
				proc.errln("Failed to open driver module '{}'.", driver_path);
				co_return 1;
			}

		}, driver);

		if (ret != 0)
		{
			proc.errln("Boot failure: driver init failure (code {}).", ret);
			os.set_state(DeviceState::Error);
			co_return 1;
		}
	}

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