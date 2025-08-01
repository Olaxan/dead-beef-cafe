#include "os_basic.h"

#include "device.h"
#include "disk.h"
#include "filesystem.h"

#include <string>
#include <vector>
#include <print>

ProcessTask Programs::CmdList(Proc& proc, std::vector<std::string> args)
{
	co_return 1;
}

ProcessTask Programs::CmdChangeDir(Proc &proc, std::vector<std::string> args)
{
	co_return 1;
}

ProcessTask Programs::CmdMakeDir(Proc& proc, std::vector<std::string> args)
{
	co_return 1;
}

ProcessTask Programs::CmdMakeFile(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	std::string curr_path = proc.get_var("SHELL_PATH");
	
	co_return 1;
}

ProcessTask Programs::CmdOpenFile(Proc& proc, std::vector<std::string> args)
{
	co_return 1;
}

ProcessTask Programs::CmdRemoveFile(Proc& proc, std::vector<std::string> args)
{
	co_return 1;
}