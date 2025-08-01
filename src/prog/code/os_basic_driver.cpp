#include "os_basic.h"

#include "device.h"
#include "netw.h"

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
	CPU* cpu = os.get_device<CPU>();

	if (cpu == nullptr)
	{
		proc.errln("fatal: no device");
		co_return 1;
	}

	proc.warnln("Found 551 orphan nodes, cleaning.");

	proc.putln("Integrated Microtrends© NT1.3");
	proc.putln("Initialising CPU microcode... (loading registers)");
	co_await os.wait(0.1f);
	proc.putln("Initialising CPU microcode... (loading instruction set)");
	co_await os.wait(0.1f);
	proc.putln("Initialising CPU microcode... (binding instruction set)");
	co_await os.wait(0.2f);
	proc.putln("Initialising CPU microcode... (compiling microcode)");
	co_await os.wait(0.3f);

	co_await os.wait(1.f);
	cpu->set_state(DeviceState::PoweredOn);
	proc.putln("[INIT] Initialised processor ({} MHz)", cpu->get_physical_speed());

	co_return 0;
}

ProcessTask Programs::InitNet(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	NIC* nic = os.get_device<NIC>();

	if (nic == nullptr)
	{
		proc.errln("Failed to initialise network interface: no device.");
		co_return 1;
	}

	co_await os.wait(1.f);
	nic->set_state(DeviceState::PoweredOn);
	proc.putln("[INIT] Initialised network interface ({} gbps)", nic->get_physical_bandwidth());
	proc.putln("Global addr: {0}", nic->get_ip());

	co_return 0;
}

ProcessTask Programs::InitDisk(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	std::vector<Disk*> disks = os.get_devices_of_type<Disk>();

	proc.putln("Found {} disk(s):", disks.size());

	if (disks.empty())
	{
		proc.errln("No disks connected!");
		co_return 1;
	}

	for (Disk* disk : os.get_devices_of_type<Disk>())
	{
		proc.putln("SuperDisk Integrated Shell");
		proc.put(" - SCAN ");
		co_await loading_bar(proc, 20, "OK");
	
		proc.put(" - SEEK ");
		co_await os.wait(0.1f);
		co_await loading_bar(proc, 20, "OK");
		proc.put("Initializing file system... ");
		co_await os.wait(0.1f);

		if (FileSystem* fs = disk->get_fs(); fs == nullptr)

		proc.put("[DONE]\nPrewarming file system access... ");
		co_await os.wait(0.2f);
		proc.putln("[DONE]");
	
		co_await os.wait(1.f);
		disk->set_state(DeviceState::PoweredOn);
		proc.putln("[INIT] Initialized disk ({} GB)", disk->get_physical_size());
	}

	co_return 0;
}
