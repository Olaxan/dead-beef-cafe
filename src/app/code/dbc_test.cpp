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

	LinkServer& links = our_world.get_link_server();

	Host* client = HostUtils::create_host<BasicOS>(our_world, "Client");
	Host* server = HostUtils::create_host<BasicOS>(our_world, "Server");

	NIC* client_nic = client->get_device<NIC>();
	NIC* server_nic = server->get_device<NIC>();
	assert(client_nic && server_nic);

	
	OS& client_os = client->get_os();
	OS& server_os = server->get_os();
	
	NetManager* client_net_mgr = client_os.get_network_manager();
	NetManager* server_net_mgr = server_os.get_network_manager();
	
	Address6 local_addr = client_net_mgr->get_primary_ip();
	Address6 remote_addr = server_net_mgr->get_primary_ip();
	
	auto sock = client_net_mgr->create_socket();
	
	if (!sock)
	return 1;
	
	SocketDescriptor fd = *sock;
	
	client->start_host();
	server->start_host();
	our_world.launch();
	
	server_os.run_process(Programs::CmdSSH, {"ssh"});
	
	links.link(client_nic, server_nic);

	/* Test ICMP */
	ip::IcmpPacket icmp;
	std::string icmp_data{};
	icmp.set_type(ip::IcmpType::EchoRequest);
	icmp.set_code(0);

	if (icmp.SerializeToString(&icmp_data))
	{
		ip::IpPackage ip;
		ip.set_dest_ip(remote_addr.raw);
		ip.set_src_ip(local_addr.raw);
		ip.set_protocol(ip::Protocol::ICMP);
		ip.set_payload(icmp_data);
	
		client_net_mgr->send(std::move(ip));
	}

	client_net_mgr->bind_socket(fd, local_addr, 50001);
	client_net_mgr->async_connect_socket(fd, local_addr, 22);

	reader(client_net_mgr, fd);

	while (client_net_mgr->socket_is_open(fd))
	{
		std::string input{};
		std::getline(std::cin, input);

		com::CommandQuery query{};
		query.set_command(input);

		std::string str{};
		if (query.SerializeToString(&str))
		 	client_net_mgr->async_write_socket(fd, str);

	}

	return 0;
}