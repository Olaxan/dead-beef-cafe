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

	MessageQueue& queue = our_os.get_msg_queue();

	our_host.start_host();
	
	std::thread our_runner([&our_host] { our_host.launch(); });

	while (true)
	{
		std::cout << our_host.get_hostname() << "> ";
		std::string input{};
		std::getline(std::cin, input);
		queue.push(input);
	}

	our_runner.join();

	return 0;
}