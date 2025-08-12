#include "os_basic.h"
#include "os_input.h"

ProcessTask Programs::CmdLogin(Proc& proc, std::vector<std::string> args)
{
	proc.put("Username: ");
	std::string username = co_await CmdInput::read_cmd_utf8(proc);

	proc.put("Password: ");
	std::string password = co_await CmdInput::read_cmd_utf8(proc);

	proc.warnln("Invalid credentials.");
	co_return 1;
}
