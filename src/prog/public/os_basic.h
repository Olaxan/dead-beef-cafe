#pragma once

#include "os.h"
#include "proc.h"

#include <coroutine>
#include <vector>

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
	ProcessTask CmdSSH(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdSudo(Proc& proc, std::vector<std::string> args);

	/* User management task */
	ProcessTask CmdLogin(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdUserAdd(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdGroupAdd(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdUser(Proc& proc, std::vector<std::string> args);

	/* Drivers */
	ProcessTask InitCpu(Proc& proc, std::vector<std::string> args);
	ProcessTask InitNet(Proc& proc, std::vector<std::string> args);
	ProcessTask InitDisk(Proc& proc, std::vector<std::string> args);
	ProcessTask InitProg(Proc& proc, std::vector<std::string> args);
	
	ProcessTask SrvNetRx(Proc& proc, std::vector<std::string> args);
	ProcessTask SrvNetTx(Proc& proc, std::vector<std::string> args);

	/* File management */
	ProcessTask CmdList(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdMakeDir(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdMakeFile(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdOpenFile(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdRemoveFile(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdCat(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdEdit(Proc& proc, std::vector<std::string> args);

	/* Test programs */
	ProcessTask CmdSnake(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdDogs(Proc& proc, std::vector<std::string> args);

	/* Networks programs */
	ProcessTask CmdPing(Proc& proc, std::vector<std::string> args);
}

class BasicOS : public OS
{
public:

	BasicOS() = delete;
	BasicOS(Host& owner);

	virtual void start_os() override;
	
};