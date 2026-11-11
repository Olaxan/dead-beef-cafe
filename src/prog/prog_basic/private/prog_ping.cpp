#include "prog_basic.h"

#include "os.h"
#include "net_types.h"
#include "filesystem.h"
#include "addr.h"
#include "net_mgr.h"
#include "race_awaiter.h"

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

	Address6 dest = parse_res.value();
	Address6 src = netapi.get_primary_ip();

	proc.putln("Pinging {} with {} bytes of data...", dest, params.payload);

	ip::IcmpPacket icmp;
	icmp.set_type(ip::IcmpType::EchoRequest);

	ip::IpPackage package;
	package.set_src_ip(src.data_str());
	package.set_dest_ip(dest.data_str());
	package.set_protocol(ip::Protocol::ICMP);
	package.set_payload(icmp.SerializeAsString());

	auto sock_res = netapi.create_socket(SocketType::Raw);
	if (not sock_res)
	{
		proc.errln("ping: {}.", sock_res.error().message());
		co_return 1;
	}

	FileDescriptor fd = sock_res.value();

	co_await netapi.async_write_socket_raw(fd, std::move(package));

	bool success = false;

	auto res = co_await when_any(netapi.async_read_socket_raw(fd), proc.wait(2.f));
	if (res.index == 0)
	{
		proc.putln("Received echo reply!");
		success = true;
	}
	else
	{
		proc.putln("Ping timed out.");
	}

	proc.putln("Host {}." CSI_RESET, success ? CSI_CODE(32) "reachable" : CSI_CODE(31) "unreachable");

	co_return 0;
}