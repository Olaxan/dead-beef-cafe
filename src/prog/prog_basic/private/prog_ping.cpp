#include "prog_basic.h"

#include "os.h"
#include "net_types.h"
#include "filesystem.h"
#include "addr.h"
#include "net_mgr.h"

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
	ProcNetApi& netapi = proc.net;;

	CLI::App app{"Network utility to test connectivity and analyze routing."};
	app.allow_windows_style_options(false);

	struct PingArgs
	{
		std::string addr;
		int32_t payload{0};
	} params{};


	app.add_option("TARGET", params.addr, "Target address for ping");
	app.add_option("-p,--payload-size", params.payload, "Size of ping payload")->default_val(32);

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

	auto parse_res = Address6::from_string(params.addr);
	
	if (!parse_res)
	{
		proc.warnln("ping: {}.", parse_res.error().what());
		co_return 1;
	}

	const Address6& dest = parse_res.value();
	Address6 src = netapi.get_primary_ip();

	proc.putln("Pinging {} with {} bytes of data...", dest, params.payload);

	ip::IpPackage package;
	package.set_src_ip(src.data_str());
	package.set_dest_ip(dest.data_str());
	package.set_protocol(ip::Protocol::ICMP);

	auto sock_res = netapi.create_socket();
	if (!sock_res)
	{
		proc.errln("ping: {}.", sock_res.error().message());
		co_return 1;
	}

	FileDescriptor fd = sock_res.value();

	netapi.async_write_socket_raw(fd, std::move(package));

	co_await os.wait(1.f);

	bool success = false;
	proc.putln("Host {}.", success ? "reachable" : "unreachable");

	co_return 0;
}