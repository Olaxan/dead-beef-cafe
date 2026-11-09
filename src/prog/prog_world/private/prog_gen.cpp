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

	Internet& internet = world.get_internet();

	internet.link(local_os.get_uid(), middle_os.get_uid());
	internet.link(middle_os.get_uid(), remote_os.get_uid());

	remote->start_host();
	middle->start_host();

	proc.putln("Created remote host with UID: {}, IP: {}", remote_os.get_uid(), remote_os.get_network_manager()->get_primary_ip());
	proc.putln("Created middle host with UID: {}, IP: {}", middle_os.get_uid(), middle_os.get_network_manager()->get_primary_ip());
	proc.putln("We are UID: {}, IP: {}.", local_os.get_uid(), local_os.get_network_manager()->get_primary_ip());

	co_return 0;
}
