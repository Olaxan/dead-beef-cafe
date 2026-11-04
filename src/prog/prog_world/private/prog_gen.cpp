#include "prog_world.h"

#include "proc.h"
#include "world.h"
#include "host_utils.h"
#include "os.h"

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

	Host* remote = HostUtils::create_host<OS>(world, "remote");
	Host* middle = HostUtils::create_host<OS>(world, "middle");
	OS& remote_os = remote->get_os();
	OS& middle_os = middle->get_os();

	Internet& internet = world.get_internet();

	internet.link(remote_os.get_uid(), middle_os.get_uid());

	proc.putln("Created remote host with UID: {}", remote_os.get_uid());
	proc.putln("Created middle host with UID: {}", middle_os.get_uid());

	co_return 0;
}
