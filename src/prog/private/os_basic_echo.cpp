#include "os_basic.h"

#include "proc.h"

#include "CLI/CLI.hpp"

#include <string>
#include <cstdlib>
#include <print>

ProcessTask Programs::CmdEcho(Proc& proc, std::vector<std::string> args)
{
	proc.putln("echo: {0}", args);
	co_return 0;
}