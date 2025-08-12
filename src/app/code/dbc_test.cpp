#include "world.h"
#include "host.h"
#include "os.h"
#include "msg_queue.h"
#include "netw.h"
#include "host_utils.h"

#include "os_basic.h"
#include "os_input.h"
#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <optional>
#include <coroutine>
#include <memory>

EagerTask<int32_t> reader(std::shared_ptr<CmdSocketClient> read_socket)
{
	while (read_socket->is_open())
	{
		com::CommandReply reply = co_await read_socket->async_read();
		std::print("{0}", reply.reply());
	}
	co_return 0;
}

int main(int argc, char* argv[])
{
	World our_world{};
	Host* our_host = HostUtils::create_host<BasicOS>(our_world, "MyComputer");
	OS& our_os = our_host->get_os();

	Proc* our_shell = our_os.get_shell(); // shell proc starts a acceptor socket at 22
	auto our_sock = our_os.create_socket<CmdSocketClient>(); 
	our_os.connect_socket<CmdSocketServer>(our_sock, our_os.get_global_ip(), 22);

	//WorldUpdateQueue& queue = our_world.get_update_queue();

	our_host->start_host();
	our_world.launch();

	//std::jthread(reader).detach();

	reader(our_sock);

	while (our_sock->is_open())
	{
		std::string input{};
		std::getline(std::cin, input);

		com::CommandQuery query{};
		query.set_command(input);

		our_sock->write(std::move(query));
	}

	return 0;
}