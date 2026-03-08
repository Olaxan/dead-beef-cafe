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

EagerTask<int32_t> reader(NetManager* net_mgr, SocketDescriptor read_socket)
{
	while (net_mgr->socket_is_open(read_socket))
	{
		std::string str = co_await net_mgr->async_read_socket(read_socket);

		com::CommandReply reply;
		if (reply.ParseFromString(str))
			std::print("{0}", reply.reply());
		else
			std::print(std::cerr, "Parse error: {0}.", str);
	}
	co_return 0;
}

int main(int argc, char* argv[])
{
	World our_world{};
	Host* our_host = HostUtils::create_host<BasicOS>(our_world, "MyComputer");
	OS& our_os = our_host->get_os();
	NetManager* net = our_os.get_network_manager();

	auto sock = net->create_socket();

	if (!sock)
		return 1;

	SocketDescriptor fd = *sock;

	net->connect_socket(fd, net->get_primary_ip(), 22);

	our_host->start_host();
	our_world.launch();

	our_os.run_process(Programs::CmdSSH, {"ssh"});

	reader(net, fd);

	while (net->socket_is_open(fd))
	{
		std::string input{};
		std::getline(std::cin, input);

		com::CommandQuery query{};
		query.set_command(input);

		std::string str{};
		if (query.SerializeToString(&str))
			net->async_write_socket(fd, str);

	}

	return 0;
}