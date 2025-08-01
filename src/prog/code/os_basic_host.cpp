#include "os_basic.h"

#include "device.h"
#include "netw.h"

#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <print>

ProcessTask Programs::CmdBoot(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	if (DeviceState state = os.get_state(); state != DeviceState::PoweredOff)
	{
		proc.errln("[ERROR] boot failure: state='{}'.", DeviceUtils::get_state_name(state));
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

		if (int32_t ret = co_await proc.exec(driver); ret != 0)
		{
			proc.errln("[ERROR] boot failure: driver init failure (code {})", ret);
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

	co_return 0;
}

ProcessTask Programs::CmdShell(Proc& proc, std::vector<std::string> args)
{
	OS& our_os = *proc.owning_os;
	auto sock = our_os.create_socket<CmdSocketServer>();
	our_os.bind_socket(sock, 22);

	/* Replace the writer functor in the process so that output
	is delivered in the form of command objects via the socket. */
	proc.writer = [sock](const std::string& out_str)
	{
		com::CommandReply reply{};
		reply.set_reply(out_str);
		sock->write_one(std::move(reply));
	};

	while (true)
	{
		DeviceState state = our_os.get_state();
		proc.put("{0}{1}> ", our_os.get_hostname(), (state != DeviceState::PoweredOn ? " (\x1B[33mNetBIOS\x1B[0m)" : ""));
		com::CommandQuery input = co_await sock->read_one();
		int32_t ret = co_await proc.exec(input.command());
		proc.putln("Process returned with code \x1B[{}m{}\x1B[0m.", (ret == 0 ? 32 : 31), ret);
	}

	co_return 0;
}