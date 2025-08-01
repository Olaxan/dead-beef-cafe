#include "os_basic.h"

#include "device.h"
#include "disk.h"
#include "filesystem.h"
#include "navigator.h"

#include <string>
#include <vector>
#include <print>

ProcessTask Programs::CmdList(Proc& proc, std::vector<std::string> args)
{
	if (Navigator* nav = proc.get_data<Navigator>())
	{
		proc.putln("Files in '{}': {}", nav->get_dir(), nav->get_files());
		co_return 0;
	}

	proc.errln("No file navigator.");
	co_return 1;
}

ProcessTask Programs::CmdChangeDir(Proc &proc, std::vector<std::string> args)
{
	if (args.size() != 2)
	{
		proc.warnln("Usage: cd [file/directory]");
		co_return 1;
	}

	if (Navigator* nav = proc.get_data<Navigator>())
	{
		co_return nav->enter(args[1]) ? 0 : 1;
	}

	proc.errln("No file navigator.");
	co_return 1;
}

ProcessTask Programs::CmdMakeDir(Proc& proc, std::vector<std::string> args)
{
	if (args.size() != 2)
	{
		proc.warnln("Usage: mkdir [name]");
		co_return 1;
	}

	if (Navigator* nav = proc.get_data<Navigator>())
	{
		co_return (nav->create_directory(args[1]) != 0) ? 0 : 1;
	}

	proc.errln("No file navigator.");
	co_return 1;
}

ProcessTask Programs::CmdMakeFile(Proc& proc, std::vector<std::string> args)
{
	if (args.size() != 2)
	{
		proc.warnln("Usage: touch [filename]");
		co_return 1;
	}

	if (Navigator* nav = proc.get_data<Navigator>())
	{
		co_return (nav->create_file(args[1]) != 0) ? 0 : 1;
	}

	proc.errln("No file navigator.");
	co_return 1;
}

ProcessTask Programs::CmdOpenFile(Proc& proc, std::vector<std::string> args)
{
	if (Navigator* nav = proc.get_data<Navigator>())
	{
		proc.putln("Files in '{}': {}", nav->get_dir(), nav->get_files());
		co_return 0;
	}

	proc.errln("No file navigator.");
	co_return 1;
}

ProcessTask Programs::CmdRemoveFile(Proc& proc, std::vector<std::string> args)
{
	if (args.size() != 2)
	{
		proc.warnln("Usage: rm [file/directory]");
		co_return 1;
	}

	if (Navigator* nav = proc.get_data<Navigator>())
	{
		co_return 1;
	}

	proc.errln("No file navigator.");
	co_return 1;
}

ProcessTask Programs::CmdGoUp(Proc &proc, std::vector<std::string> args)
{
	if (Navigator* nav = proc.get_data<Navigator>())
	{
		co_return nav->go_up() ? 0 : 1;
	}

	proc.errln("No file navigator.");
	co_return 1;
}
