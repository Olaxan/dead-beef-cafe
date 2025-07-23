#include "world.h"
#include "host.h"
#include "os.h"
#include "msg_queue.h"
#include "os_basic.h"

#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <optional>

int main(int argc, char* argv[])
{
	World our_world{};
	Host* our_host = our_world.create_host<BasicOS>("MyComputer");
	OS& our_os = our_host->get_os();

	Proc* our_shell = our_os.get_shell();

	//WorldUpdateQueue& queue = our_world.get_update_queue();

	our_host->start_host();
	our_world.launch();

	while (true)
	{
		std::string input{};
		std::getline(std::cin, input);
		our_shell->write(input);
		//queue.push([&our_os, input] { our_os.exec(input); });
	}

	return 0;
}