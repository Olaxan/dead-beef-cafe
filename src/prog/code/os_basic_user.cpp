#include "os_basic.h"
#include "os_input.h"

ProcessTask Programs::CmdLogin(Proc& proc, std::vector<std::string> args)
{
	CmdInput::CmdReaderParams usr_params{};
	CmdInput::CmdReaderParams pwd_params{.password = true};

	proc.put("Username: ");
	std::string username = co_await CmdInput::read_cmd_utf8(proc, usr_params);

	proc.put("Password: ");
	std::string password = co_await CmdInput::read_cmd_utf8(proc, pwd_params);

	proc.warnln("Invalid credentials.");
	co_return 1;
}
