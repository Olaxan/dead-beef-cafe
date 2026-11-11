#include "prog_world.h"

#include "proc.h"
#include "world.h"
#include "host_utils.h"
#include "os.h"
#include "os_basic.h"

#include "CLI/CLI.hpp"

#include <string>
#include <cstdlib>
#include <print>
#include <iostream>
#include <fstream>

#include <iso646.h>

ProcessTask Programs::CmdGen(Proc& proc, std::vector<std::string> args)
{
	World& world = *proc.owning_os->get_outer_as<World*>();

	Host* remote = HostUtils::create_host<BasicOS>(world, "remote");
	Host* middle = HostUtils::create_host<BasicOS>(world, "middle");
	OS& local_os = *proc.owning_os;
	OS& remote_os = remote->get_os();
	OS& middle_os = middle->get_os();
	NetManager* remote_net = remote_os.get_network_manager();
	NetManager* middle_net = middle_os.get_network_manager();
	NetManager* local_net = local_os.get_network_manager();

	//remote_net->set_primary_ip("dead::");
	//middle_net->set_primary_ip("beef::");
	//local_net->set_primary_ip("cafe::");

	Internet& internet = world.get_internet();

	internet.link(local_os.get_uid(), middle_os.get_uid());
	internet.link(middle_os.get_uid(), remote_os.get_uid());

	local_net->add_route(Address6{}, 0, middle_net->get_primary_ip(), RouteType::Static);
	middle_net->add_route(local_net->get_primary_ip(), 128, local_net->get_primary_ip(), RouteType::Static);
	middle_net->add_route(remote_net->get_primary_ip(), 128, remote_net->get_primary_ip(), RouteType::Static);
	remote_net->add_route(Address6{}, 0, middle_net->get_primary_ip(), RouteType::Static);

	remote->start_host();
	middle->start_host();

	proc.putln("Created remote host with UID: {}, IP: {}", remote_os.get_uid(), remote_os.get_network_manager()->get_primary_ip());
	proc.putln("Created middle host with UID: {}, IP: {}", middle_os.get_uid(), middle_os.get_network_manager()->get_primary_ip());
	proc.putln("We are UID: {}, IP: {}.", local_os.get_uid(), local_os.get_network_manager()->get_primary_ip());

	co_return 0;
}
