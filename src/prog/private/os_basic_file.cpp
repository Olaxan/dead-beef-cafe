#include "os_basic.h"

#include "device.h"
#include "disk.h"
#include "filesystem.h"
#include "os_fileio.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>


ProcessTask Programs::CmdOpenFile(Proc& proc, std::vector<std::string> args)
{
	proc.errln("No file navigator.");
	co_return 1;
}
