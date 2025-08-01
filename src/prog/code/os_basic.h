#pragma once

#include "os.h"
#include "proc.h"

#include <coroutine>
#include <vector>

using CmdSocketServer = Socket<com::CommandQuery, com::CommandReply>;
using CmdSocketClient = Socket<com::CommandReply, com::CommandQuery>;

namespace Programs
{
	/* Regular tasks */
	ProcessTask CmdEcho(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdCount(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdProc(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdWait(Proc& proc, std::vector<std::string> args);

	/* Host tasks */
	ProcessTask CmdBoot(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdShutdown(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdShell(Proc& proc, std::vector<std::string> args);

	/* Drivers */
	ProcessTask InitCpu(Proc& proc, std::vector<std::string> args);
	ProcessTask InitNet(Proc& proc, std::vector<std::string> args);
	ProcessTask InitDisk(Proc& proc, std::vector<std::string> args);

	/* File management */
	ProcessTask CmdList(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdChangeDir(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdMakeDir(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdMakeFile(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdOpenFile(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdRemoveFile(Proc& proc, std::vector<std::string> args);
}

class BasicOS : public OS
{
public:

	BasicOS() = delete;

	BasicOS(Host& owner) : OS(owner) 
	{ 
		register_commands(); 
		register_devices();
	};

	void register_commands() override
	{
		commands_ = {
			{"echo", Programs::CmdEcho},
			{"count", Programs::CmdCount},
			{"shutdown", Programs::CmdShutdown},
			{"wait", Programs::CmdWait},
			{"proc", Programs::CmdProc},
			{"shell", Programs::CmdShell},
			{"boot", Programs::CmdBoot },
			{"cpu", Programs::InitCpu},
			{"net", Programs::InitNet},
			{"disk", Programs::InitDisk},
			{"ls", Programs::CmdList},
			{"cd", Programs::CmdChangeDir},
			{"mkdir", Programs::CmdMakeDir},
			{"touch", Programs::CmdMakeFile},
			{"open", Programs::CmdOpenFile},
			{"rm", Programs::CmdRemoveFile}
		};
	}

protected:

	[[nodiscard]] ProcessFn get_default_shell() const override { return Programs::CmdShell; }
	
};