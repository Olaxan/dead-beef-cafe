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

	co_return 0;
}
