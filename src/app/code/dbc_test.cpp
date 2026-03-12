#include "world.h"
#include "host.h"
#include "os.h"
#include "msg_queue.h"
#include "net_types.h"
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

#include <windows.h>

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

void set_flags(DWORD& flags, DWORD flag)
{
	flags |= flag;
}

void unset_flags(DWORD& flags, DWORD flag)
{
	flags &= (~flag);
}

bool try_set_flags(DWORD handle, DWORD flags)
{
    HANDLE h = GetStdHandle(handle);
    if (h == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(h, &dwMode))
    {
        return false;
    }

	set_flags(dwMode, flags);

	if (!SetConsoleMode(h, dwMode))
    {
        return false;
    }

	return true;
}

bool try_unset_flags(DWORD handle, DWORD flags)
{
    HANDLE h = GetStdHandle(handle);
    if (h == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(h, &dwMode))
    {
        return false;
    }

	unset_flags(dwMode, flags);

	if (!SetConsoleMode(h, dwMode))
    {
        return false;
    }

	return true;
}

bool enable_vtt_mode()
{
	bool i = try_set_flags(STD_INPUT_HANDLE, ENABLE_VIRTUAL_TERMINAL_INPUT);
	bool o = try_set_flags(STD_OUTPUT_HANDLE, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	return i && o;
}

bool disable_vtt_mode()
{
	bool i = try_unset_flags(STD_INPUT_HANDLE, ENABLE_VIRTUAL_TERMINAL_INPUT);
	bool o = try_unset_flags(STD_OUTPUT_HANDLE, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	return i && o;
}

bool enable_raw_mode()
{
	return try_unset_flags(STD_INPUT_HANDLE, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
}

bool disable_raw_mode()
{
	return try_set_flags(STD_INPUT_HANDLE, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
}

std::string getch()
{
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	if (h == NULL) 
	{
		std::println("No console!");
		return {}; // console not found
	}
	DWORD count;
	TCHAR c[1024];
	ReadConsole(h, c, 1024, &count, NULL);
	std::string out;
	out.assign(c, count);
	return out;
}

/* Read Unicode (UTF-16) input from console */
std::wstring read_console_input_w() 
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    std::wstring result;
    wchar_t buffer[256];
    DWORD charsRead;

    if (ReadConsoleW(hInput, buffer, 255, &charsRead, nullptr))
	{
        buffer[charsRead] = L'\0';  // Null-terminate
        result = buffer;
    }
    return result;
}

std::string utf16_to_utf8(const std::wstring& wstr) 
{
    if (wstr.empty()) return std::string();

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
                                         nullptr, 0, nullptr, nullptr);

    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
                        &strTo[0], sizeNeeded, nullptr, nullptr);

    return strTo;
}

EagerTask<int32_t> reader(NetManager* net_mgr, OpenSocketHandle read_socket)
{
	while (net_mgr->socket_is_open(read_socket))
	{
		std::string str = co_await net_mgr->async_read_socket(read_socket);

		com::CommandReply reply;
		if (reply.ParseFromString(str))
		{
			std::cout << reply.reply();
		}
		else
		{
			std::print(std::cerr, "Parse error: {0}.", str);
		}
	}
	co_return 0;
}

int main(int argc, char* argv[])
{

	if (enable_raw_mode() == false)
	{
		std::println("Warning: Failed to configure raw terminal mode. The app will not work as intended.");
	}
	
	if (enable_vtt_mode() == false)
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
	
	server_os.run_process(Programs::CmdSSH, {"ssh"});
	
	links.link(client_nic, server_nic);

	/* Test ICMP */
	if (test_icmp)
	{
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
	}

	client_net_mgr->bind_socket(h, local_addr, 49152);
	client_net_mgr->async_connect_socket(h, remote_addr, 22);

	reader(client_net_mgr, h);

	while (client_net_mgr->socket_is_open(h))
	{
		std::wstring wide_in = read_console_input_w();
		std::string utf8_in = utf16_to_utf8(wide_in);

		if (!utf8_in.empty() && utf8_in[0] == 'Q')
			break;

		com::CommandQuery query;
		query.set_command(utf8_in);

		com::ScreenData* screen = get_screen_data();
		query.set_allocated_screen_data(screen);
		
		std::string str;
		if (query.SerializeToString(&str))
		 	client_net_mgr->async_write_socket(h, str);

	}

	return 0;
}