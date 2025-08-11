#include "os_basic.h"

#include "device.h"
#include "netw.h"
#include "proc.h"
#include "filesystem.h"

#include "CLI/CLI.hpp"

#include <string>
#include <cstdlib>
#include <print>

ProcessTask Programs::CmdEcho(Proc& proc, std::vector<std::string> args)
{
	proc.putln("echo: {0}", args);
	co_return 0;
}

ProcessTask Programs::CmdCount(Proc& proc, std::vector<std::string> args)
{
	CLI::App app{"A program that counts!"};

	float delay = 1.f;
	int32_t count = 5;
	app.add_option("-c", count, "The number to count to");
	app.add_option("-d", delay, "The time to wait per count");

	std::ranges::reverse(args);
	args.pop_back();

	try
	{
        app.parse(std::move(args));
    }
	catch(const CLI::ParseError& e)
	{
		int res = app.exit(e, proc.s_out, proc.s_err);
        co_return res;
    }

	for (int32_t i = 0; i < count; ++i)
	{
		co_await proc.owning_os->wait(delay);
		proc.putln("{0}...", i + 1);
	}

	co_return 0;
}

ProcessTask Programs::CmdProc(Proc& proc, std::vector<std::string> args)
{
	proc.putln("Processes on {0}:", proc.owning_os->get_hostname());
	proc.owning_os->get_processes([&proc](const Proc& process)
	{
		proc.putln("{0}: '{1}'", process.get_pid(), process.get_name());
	});
	co_return 0;
}

ProcessTask Programs::CmdWait(Proc& proc, std::vector<std::string> args)
{
	if (args.size() < 2)
	{
		proc.putln("Usage: wait [time (s)]");
		co_return 1;
	}

	float delay = static_cast<float>(std::atof(args[1].c_str()));
	co_await proc.owning_os->wait(delay);

	proc.putln("Waited {0} seconds.", delay);
	co_return 0;
}

BasicOS::BasicOS(Host& owner) : OS(owner)
{
	register_devices();

	FileSystem* fs = get_filesystem();
	if (fs == nullptr)
		return;

	std::vector<std::pair<std::string, ProcessFn>> os_progs = 
	{
		{"/bin/..", Programs::CmdGoUp},
		{"/bin/proc", Programs::CmdProc},
		{"/bin/wait", Programs::CmdWait},
		{"/sbin/shutdown", Programs::CmdShutdown},
		{"/sbin/shell", Programs::CmdShell},
		{"/sbin/boot", Programs::CmdBoot },
		{"/bin/ls", Programs::CmdList},
		{"/bin/cd", Programs::CmdChangeDir},
		{"/bin/mkdir", Programs::CmdMakeDir},
		{"/bin/touch", Programs::CmdMakeFile},
		{"/bin/open", Programs::CmdOpenFile},
		{"/bin/echo", Programs::CmdEcho},
		{"/bin/rm", Programs::CmdRemoveFile},
		{"/usr/bin/count", Programs::CmdCount},
		{"/usr/bin/snake", Programs::CmdSnake},
		{"/usr/bin/dogs", Programs::CmdDogs},
		{"/usr/bin/edit", Programs::CmdEdit},
		{"/lib/modules/kernel/drivers/cpu", Programs::InitCpu},
		{"/lib/modules/kernel/drivers/net", Programs::InitNet},
		{"/lib/modules/kernel/drivers/disk", Programs::InitDisk}
	};

	for (auto& fn : os_progs)
	{
		if (auto [fid, ptr, err] = fs->create_file(fn.first); err == FileSystemError::Success)
		{
			ptr->write(std::forward<ProcessFn>(fn.second));
			ptr->set_flag(FileModeFlags::Execute);
		}
	}


}
