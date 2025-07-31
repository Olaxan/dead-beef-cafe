#include "os_basic.h"

#include "device.h"
#include "netw.h"

#include "cpu.h"
#include "disk.h"
#include "nic.h"

#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <print>

EagerTask<int32_t> loading_bar(Proc& proc, std::size_t length, std::string result)
{
	proc.put("[");
	for (std::size_t i = 0; i < length; ++i)
	{
		proc.put("I");
		co_await proc.owning_os->wait(0.05f);
	}
	proc.putln("] [{}]", result);
	co_return 0;
}

ProcessTask Programs::InitCpu(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	CPU* cpu = os.get_device<CPU>();

	proc.putln("Integrated Microtrends© NT1.3");
	proc.putln("Initialising CPU microcode... (loading registers)");
	co_await os.wait(0.1f);
	proc.putln("Initialising CPU microcode... (loading instruction set)");
	co_await os.wait(0.1f);
	proc.putln("Initialising CPU microcode... (binding instruction set)");
	co_await os.wait(0.2f);
	proc.putln("Initialising CPU microcode... (compiling microcode)");
	co_await os.wait(0.3f);

	if (cpu == nullptr)
	{
		proc.putln("fatal: ");
		co_return 1;
	}

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
		proc.putln("[ERROR] No such device (NIC)");
		co_return 1;
	}

	co_await os.wait(1.f);
	nic->set_state(DeviceState::PoweredOn);
	proc.putln("[INIT] Initialised network interface ({} gbps)", nic->get_physical_bandwidth());

	co_return 0;
}

ProcessTask Programs::InitDisk(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	Disk* disk = os.get_device<Disk>();

	proc.putln("SuperDisk Integrated Shell");
	proc.put(" - SCAN ");
	co_await loading_bar(proc, 20, "OK");

	proc.put(" - SEEK ");
	co_await os.wait(0.1f);
	co_await loading_bar(proc, 20, "OK");

	proc.put("Initialising file system... ");
	co_await os.wait(0.1f);
	proc.put("[DONE]\nPrewarming file system access... ");
	co_await os.wait(0.2f);
	proc.putln("[DONE]");

	if (disk == nullptr)
	{
		proc.putln("[ERROR] No such device");
		co_return 1;
	}

	co_await os.wait(1.f);
	disk->set_state(DeviceState::PoweredOn);
	proc.putln("[INIT] Initialised disk ({} GB)", disk->get_physical_size());

	co_return 0;
}