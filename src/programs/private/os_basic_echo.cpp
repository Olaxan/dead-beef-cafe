#include "os_basic.h"

#include "proc.h"

#include "CLI/CLI.hpp"

#include <string>
#include <cstdlib>
#include <print>

ProcessTask Programs::CmdEcho(Proc& proc, std::vector<std::string> args)
{
	auto str = args
	| std::views::drop(1)
	| std::views::join_with(' ')
	| std::ranges::to<std::string>();

	proc.putln("{}", str);
	co_return 0;
}