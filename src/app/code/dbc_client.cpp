#include "dbc.h"
#include "dbc_win.h"
#include "msg_queue.h"
#include "world.h"
#include "host.h"
#include "os.h"
#include "msg_queue.h"
#include "os_basic.h"
#include "os_input.h"
#include "host_utils.h"
#include "uuid.h"

#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include "google/protobuf/util/delimited_message_util.h"

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <string>
#include <print>
#include <algorithm>

#include <asio.hpp>

#include <windows.h>

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::redirect_error;
using asio::use_awaitable;




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
		std::println("Warning: Failed to configure raw terminal mode. The game will exit.");
		return 1;
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

	Host* client = HostUtils::create_host<BasicOS>(our_world, "Client");
	NIC* client_nic = client->get_device<NIC>();
	OS& client_os = client->get_os();
	FileSystem* client_fs = client_os.get_filesystem();
	NetManager* client_net_mgr = client_os.get_network_manager();
	assert(client_nic && client_net_mgr && client_fs);
	
	Address6 local_addr = client_net_mgr->get_primary_ip();
	
	client->start_host();

	client_fs->create_file("/bin/join", 
	{
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		},
		.executable = Programs::CmdDbcClient
	});

	client_fs->create_file("/bin/host", 
	{
		.recurse = true,
		.meta = {
			.perm_owner = FilePermissionTriad::All,
			.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
			.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute
		},
		.executable = Programs::CmdDbcServer
	});

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