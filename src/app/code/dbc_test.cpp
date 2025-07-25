#include "world.h"
#include "host.h"
#include "os.h"
#include "msg_queue.h"
#include "netw.h"

#include "os_basic.h"
#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <optional>
#include <coroutine>

int main(int argc, char* argv[])
{
	World our_world{};
	Host* our_host = our_world.create_host<BasicOS>("MyComputer");
	OS& our_os = our_host->get_os();

	Proc* our_shell = our_os.get_shell();
	auto* our_sock = our_os.create_socket<CmdSocketClient>(22/*, our_os.get_ip()*/); //, 
	our_sock->connect(); //connect our fake 

	//WorldUpdateQueue& queue = our_world.get_update_queue();

	our_host->start_host();
	our_world.launch();

	auto reader = [our_sock]() -> EagerTask<int32_t>
	{
		while (our_sock->is_open())
		{
			com::CommandReply reply = co_await our_sock->read_one();
			std::println(reply.reply());
		}
	};

	std::jthread(reader).detach();

	while (our_sock->is_open())
	{
		std::string input{};
		std::getline(std::cin, input);

		com::CommandQuery query{};
		query.set_command(input);

		our_sock->write_one(std::move(query));
	}

	return 0;
}