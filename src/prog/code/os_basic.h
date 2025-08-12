#pragma once

#include "os.h"
#include "proc.h"
#include "netw.h"

#include <coroutine>
#include <vector>

using CmdSocketServer = Socket<com::CommandQuery, com::CommandReply>;
using CmdSocketClient = Socket<com::CommandReply, com::CommandQuery>;
using CmdSocketAwaiterServer = MessageAwaiter<com::CommandQuery, com::CommandReply>;
using CmdSocketAwaiterClient = MessageAwaiter<com::CommandReply, com::CommandQuery>;

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
	ProcessTask InitProg(Proc& proc, std::vector<std::string> args);

	/* File management */
	ProcessTask CmdList(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdChangeDir(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdMakeDir(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdMakeFile(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdOpenFile(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdRemoveFile(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdCat(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdGoUp(Proc& proc, std::vector<std::string> args);

	/* Test programs */
	ProcessTask CmdSnake(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdDogs(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdEdit(Proc& proc, std::vector<std::string> args);
}

class BasicOS : public OS
{
public:

	BasicOS() = delete;
	BasicOS(Host& owner);

protected:

	[[nodiscard]] ProcessFn get_default_shell() const override { return Programs::CmdShell; }
	
};