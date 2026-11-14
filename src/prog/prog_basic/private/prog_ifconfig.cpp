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

ProcessTask Programs::CmdIfConfig(Proc& proc, std::vector<std::string> args)
{
	OS* our_os = proc.owning_os;
	NetManager* netmgr = our_os->get_network_manager();

	CLI::App app{"Configuration utility to analyze and change network settings."};
	app.allow_windows_style_options(false);

	struct IfConfigArgs
	{
		// None.
	} params{};

	//app.add_option("sample", params.addr, "Sample option");

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

	proc.putln("Link-local address: {}.", proc.net.get_primary_ip());

	proc.putln("Routing table:");
	proc.putln("{:<43} {:<39} {}", "Destination", "Gateway", "Type");
	for (const RoutingTableEntry& route : netmgr->get_routes())
	{
		const char* type = route.type == RouteType::Static ? "static" : "dynamic";
		proc.putln("{:<43} {:<39} {}", std::format("{}/{}", route.address, route.prefix), route.via, type);
	}

	proc.putln("");
	proc.putln("ARP cache:");
	proc.putln("{:<39} {}", "Address", "HWaddress");
	for (const auto& [address, mac] : netmgr->get_arp_cache())
	{
		proc.putln("{:<39} {}", address, mac);
	}
	
    co_return 0;
}