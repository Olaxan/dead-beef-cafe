#include "os_basic.h"

#include "proc.h"

#include "CLI/CLI.hpp"

#include <string>

ProcessTask Programs::CmdProc(Proc& proc, std::vector<std::string> args)
{
	proc.putln("PID: {}, SID: {}, UID: {}, GID: {}",
		proc.get_pid(), proc.get_sid(), proc.get_uid(), proc.get_gid());

	proc.putln("Processes on {0}:", proc.owning_os->get_hostname());
	proc.owning_os->get_processes([&proc](const Proc& process)
	{
		proc.putln("{0}: '{1}'", process.get_pid(), process.get_name());
	});
	co_return 0;
}