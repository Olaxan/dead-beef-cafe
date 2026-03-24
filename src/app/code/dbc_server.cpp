#include "dbc.h"
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

	Host* server = HostUtils::create_host<BasicOS>(our_world, "Server");

	NIC* server_nic = server->get_device<NIC>();
	OS& server_os = server->get_os();
	NetManager* server_net_mgr = server_os.get_network_manager();
	assert(server_nic && server_net_mgr);
	
	Address6 remote_addr = server_net_mgr->get_primary_ip();
	
	server->start_host();
	our_world.launch();
	
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

	int32_t shell_pid{-1};
	InvokeFn local_invoke = [&shell_pid](Proc* proc) 
	{
		shell_pid = proc->get_pid();
	};

	server_os.run_process(Programs::CmdDbcServer, {"host"}, OS::CreateProcessParams
	{
		.invoke = std::move(local_invoke),
		.writer = std::move(local_writer),
	});

	while (server_os.process_is_running(shell_pid))
	{
		std::wstring wide_in = DbcWin::read_console_input_w();
		std::string utf8_in = DbcWin::utf16_to_utf8(wide_in);

		if (!utf8_in.empty() && utf8_in[0] == 'Q')
			break;
	}

	return 0;
}