#include "os_basic.h"

#include "device.h"
#include "netw.h"
#include "filesystem.h"
#include "navigator.h"

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

	FileSystem* fs = our_os.get_filesystem();

	if (fs == nullptr)
	{
		proc.warnln("No file system!");
		co_return 1;
	}

	Navigator nav(*fs);
	proc.set_data(&nav);

	while (true)
	{
		proc.set_var("SHELL_PATH", nav.get_path());

		std::string usr_str = TermUtils::color(std::format("usr@{}", our_os.get_hostname()), TermColor::BrightMagenta);
		std::string path_str = TermUtils::color(nav.get_path(), TermColor::BrightBlue);
		std::string net_str = (our_os.get_state() != DeviceState::PoweredOn) ? "(\x1B[33mNetBIOS\x1B[0m) " : "";

		proc.put("{0}{1}:{2}$ ", net_str, usr_str, path_str); 

		com::CommandQuery input = co_await sock->read_one();
		int32_t ret = co_await proc.exec(input.command());

		if (input.has_screen_data())
		{
			com::ScreenData screen = input.screen_data();
			auto const current_time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
			std::string ret_sym = std::format(CSI_PLACEHOLDER "{}" CSI_RESET, (ret == 0 ? 32 : 31), (ret == 0 ? "✓" : "✕"));
			std::string time_str = std::format("{:%Y-%m-%d %X}", current_time);
			std::string line_str = std::format("{} {}", ret_sym, time_str);
			int32_t length = 2 + time_str.length();

			proc.putln("\n{}", TermUtils::msg_line(line_str, screen.size_x() - length));
		}

		//proc.put("\x1B[{}m{}\x1B[0m ", , );
		proc.set_var("RET_VAL", ret);
	}

	co_return 0;
}