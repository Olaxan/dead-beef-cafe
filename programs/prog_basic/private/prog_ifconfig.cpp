#include "os_basic.h"

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
	proc.putln("Link-local address: {}.", proc.net.get_primary_ip());
	
    co_return 0;
}