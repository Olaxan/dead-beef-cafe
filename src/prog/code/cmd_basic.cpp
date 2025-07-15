#include "cmd_basic.h"
#include "cmd.h"
#include "os.h"

#include <iostream>
#include <vector>
#include <string>


void CmdEcho::parse(OS &os, std::string_view cmd)
{
	std::vector<std::string> boo = lex(cmd);

	for (auto& elem : boo)
		std::cout << elem << std::endl;

	std::cout << std::endl;
}

void CmdShutdown::parse(OS &runner, std::string_view cmd)
{
	runner.shutdown_os();
}
