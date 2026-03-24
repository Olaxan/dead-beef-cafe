#include "dbc_win.h"
#include "world.h"
#include "host.h"
#include "os.h"
#include "msg_queue.h"
#include "net_types.h"
#include "host_utils.h"

#include "os_basic.h"
#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <optional>
#include <coroutine>
#include <memory>

#include <windows.h>
#include <iso646.h>

constexpr bool test_icmp = false;


com::ScreenData* get_screen_data()
{
	HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen_info;
	GetConsoleScreenBufferInfo(h_out, &screen_info);
	COORD screen_size;
	screen_size.X = screen_info.srWindow.Right - screen_info.srWindow.Left + 1;
	screen_size.Y = screen_info.srWindow.Bottom - screen_info.srWindow.Top + 1;
	com::ScreenData* screen = new com::ScreenData();
	screen->set_size_x(screen_size.X);
	screen->set_size_y(screen_size.Y);

	return screen;
}

int main(int argc, char* argv[])
{

	if (DbcWin::enable_raw_mode() == false)
	{
		std::println("Warning: Failed to configure raw terminal mode. The app will not work as intended.");
	}
	
	if (DbcWin::enable_vtt_mode() == false)
	{
		std::println("Warning: Failed to configure virtual terminal mode. The app might not work as intended.");
	}
	
	if (SetConsoleOutputCP(CP_UTF8) == false)
	{
		std::println("Warning: Failed to configure utf-8 terminal mode. The app might not work as intended.");
	}

	World our_world{};

	LinkServer& links = our_world.get_link_server();

	Host* client = HostUtils::create_host<BasicOS>(our_world, "Client");
	Host* server = HostUtils::create_host<BasicOS>(our_world, "Server");

	NIC* client_nic = client->get_device<NIC>();
	NIC* server_nic = server->get_device<NIC>();
	assert(client_nic && server_nic);

	//client_nic->set_ip("acad::");
	//server_nic->set_ip("dead:beef:cafe::");
	
	OS& client_os = client->get_os();
	OS& server_os = server->get_os();
	
	NetManager* client_net_mgr = client_os.get_network_manager();
	NetManager* server_net_mgr = server_os.get_network_manager();
	
	Address6 local_addr = client_net_mgr->get_primary_ip();
	Address6 remote_addr = server_net_mgr->get_primary_ip();
	
	auto sock = client_net_mgr->create_socket();
	
	if (!sock)
	return 1;
	
	OpenSocketHandle h = sock->first;
	
	client->start_host();
	server->start_host();
	our_world.launch();
	
	server_os.run_process(Programs::CmdSshServer, {"ssh"});
	
	links.link(client_nic, server_nic);

	auto queue_ptr = std::make_shared<MessageQueue<std::string>>();

	WriterFn local_writer = [](const Proc&, const std::string& str)
	{
		com::CommandReply rep;
		if (rep.ParseFromString(str))
		{
			std::cout << rep.reply();
		}
		else
		{
			std::cout << str;
		}
	};

	ReaderFn local_reader = [q = queue_ptr](const Proc&) -> Task<ReadResult>
	{
		co_return (co_await q->async_pop());
	};

	int32_t shell_pid{-1};

	InvokeFn local_invoke = [&shell_pid](Proc* proc) 
	{
		shell_pid = proc->get_pid();
	};

	client_os.run_process(Programs::CmdShell, {"shell"}, OS::CreateProcessParams
	{
		.invoke = std::move(local_invoke),
		.writer = std::move(local_writer),
		.reader = std::move(local_reader),
	});

	//reader(client_net_mgr, h);

	while (client_os.process_is_running(shell_pid))
	{
		std::wstring wide_in = DbcWin::read_console_input_w();
		std::string utf8_in = DbcWin::utf16_to_utf8(wide_in);

		if (!utf8_in.empty() && utf8_in[0] == 'Q')
			break;

		com::CommandQuery query;
		query.set_command(utf8_in);

		com::ScreenData* screen = get_screen_data();
		query.set_allocated_screen_data(screen);
		
		std::string str;
		if (query.SerializeToString(&str))
		 	queue_ptr->push(std::move(str));

	}

	return 0;
}