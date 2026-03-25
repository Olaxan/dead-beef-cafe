#include "os_basic.h"

#include "os.h"
#include "filesystem.h"
#include "sha256.h"
#include "users_mgr.h"

#include "CLI/CLI.hpp"

#include <ranges>
#include <charconv>
#include <unordered_set>


ProcessTask Programs::CmdGroupAdd(Proc& proc, std::vector<std::string> args)
{
	co_return 1;
}