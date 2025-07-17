#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <optional>

#include "world.h"
#include "host.h"
#include "os.h"

#include "msg_queue.h"

#include "prog1.h"


int main(int argc, char* argv[])
{
	World our_world{};
	Host& our_host = our_world.create_host("MyComputer");
	OS& our_os = our_host.get_os();
	
	our_os.add_command("echo", Programs::CmdEcho);
	our_os.add_command("shutdown", Programs::CmdShutdown);
	our_os.add_command("count", Programs::CmdCount);
	our_os.add_command("proc", Programs::CmdProc);
	our_os.add_command("wait", Programs::CmdWait);

	WorldUpdateQueue& queue = our_world.get_update_queue();

	our_host.start_host();
	our_world.launch();

	while (true)
	{
		std::cout << our_host.get_hostname() << "> ";
		std::string input{};
		std::getline(std::cin, input);
		queue.push([&our_os, input] { our_os.exec(input); });
	}

	return 0;
}