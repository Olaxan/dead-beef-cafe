#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <string>
#include <print>

#include <asio.hpp>

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::redirect_error;
using asio::use_awaitable;

void stop(tcp::socket& socket)
{
	socket.close();
}

// awaitable<void> reader(tcp::socket& socket)
// {
// 	try
// 	{
// 		// for (std::string read_msg;;)
// 		// {
// 		// 	std::size_t n = co_await asio::async_read_until(socket, asio::dynamic_buffer(read_msg, 1024), "\n", use_awaitable);

// 		// 	//room_.deliver(read_msg.substr(0, n));
// 		// 	read_msg.erase(0, n);
// 		// }
// 	}
// 	catch (std::exception&)
// 	{
// 		stop(socket);
// 	}
// }

// awaitable<void> writer(tcp::socket& socket)
// {
// 	try
// 	{
// 		while (socket.is_open())
// 		{
// 			// if (write_msgs_.empty())
// 			// {
// 			// 	asio::error_code ec;
// 			// 	//co_await timer_.async_wait(redirect_error(use_awaitable, ec));
// 			// }
// 			// else
// 			// {
// 			// 	co_await asio::async_write(socket, asio::buffer(write_msgs_.front()), use_awaitable);
// 			// 	write_msgs_.pop_front();
// 			// }
// 		}
// 	}
// 	catch (std::exception&)
// 	{
// 		stop(socket);
// 	}
// }

awaitable<void> connect(tcp::socket socket, char* addr, char* service)
{
	auto& context = socket.get_executor();
	std::error_code ec;
	tcp::resolver res(context);
	std::deque<std::string> commands;

	std::println("Connecting to {0}:{1}...", addr, service);
	
	auto resolved = co_await res.async_resolve(addr, service, asio::redirect_error(asio::use_awaitable, ec));
	co_await asio::async_connect(socket, resolved, asio::redirect_error(asio::use_awaitable, ec));

	//co_spawn(context, reader(socket), detached);
	//co_spawn(context, writer(socket), detached);

	if (!ec)
		std::cout << "Connected to " << socket.remote_endpoint() << ".\n";

	while (true)
	{
		std::cout << "Enter message: ";
		std::string str{};
		std::getline(std::cin, str);

		if (strcmp(str.c_str(), "exit") == 0)
		{
			socket.close();
			break;
		}

		str.append("\n");

		co_await asio::async_write(socket, asio::buffer(str), use_awaitable);
	}
}

int main(int argc, char* argv[])
{
	try
	{

		if (argc != 3)
		{
			std::cerr << "Usage: dbc_client <host> <port>\n";
			return 1;
		}

		asio::io_context io_context(1);
		co_spawn(io_context, connect(tcp::socket(io_context), argv[1], argv[2]), detached);

		asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&](auto, auto) { io_context.stop(); });
		
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}