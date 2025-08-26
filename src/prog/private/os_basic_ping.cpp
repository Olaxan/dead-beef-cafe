#include "os_basic.h"

#include "os.h"
#include "netw.h"
#include "filesystem.h"
#include "os_input.h"
#include "os_fileio.h"
#include "os_netio.h"
#include "addr.h"

#include "CLI/CLI.hpp"

#include "proto/ip_packet.pb.h"

#include <string>
#include <vector>
#include <print>
#include <format>
#include <ranges>
#include <functional>

ProcessTask Programs::CmdPing(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();
	assert(fs);

	CLI::App app{"Network utility to test connectivity and analyze routing."};
	app.allow_windows_style_options(false);

	struct PingArgs
	{
		std::string addr;
	} params{};


	app.add_option("TARGET", params.addr, "Target address for ping");

	try
	{
		std::ranges::reverse(args);
		args.pop_back();
        app.parse(std::move(args));
    }
	catch(const CLI::ParseError& e)
	{
		int res = app.exit(e, proc.s_out, proc.s_err);
        co_return res;
    }

	try
	{
		Address6 src = os.get_global_ip();
		Address6 dest{params.addr};
		proc.putln("Pinging {} with 32 bytes of data...", dest);
		ip::IpPackage package;
		package.set_dest_ip(dest.raw);
		package.set_src_ip(src.raw);
		package.set_protocol(ip::Protocol::ICMP);
		co_await os.wait(1.f);

		//auto [conn, err] = NetUtils::connect(proc, dest, 22);


		co_return 0;
	}
	catch (const std::invalid_argument& e)
	{
		proc.warnln("ping: {}", e.what());
		co_return 1;
	}

	co_return 1;
}