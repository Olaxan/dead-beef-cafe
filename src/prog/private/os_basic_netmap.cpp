#include "os_basic.h"

#include "os.h"
#include "net_types.h"
#include "filesystem.h"
#include "os_fileio.h"
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


ProcessTask Programs::CmdNetMap(Proc& proc, std::vector<std::string> args)
{
	proc.errln("Not implemented.");
    co_return 1;
}