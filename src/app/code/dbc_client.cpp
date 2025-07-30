#include "proto/test.pb.h"
#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include "google/protobuf/util/delimited_message_util.h"

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

awaitable<void> reader(tcp::socket& socket)
{
	try
	{
		constexpr std::size_t header_size = sizeof(int32_t);

		asio::streambuf stream{};
		std::istream input_stream(&stream);

		while (true)
		{
			stream.prepare(header_size);
			auto header_bytes = co_await asio::async_read(socket, stream, use_awaitable);
			std::println("Preparing to read {0} bytes ({1} expected).", header_bytes, header_size);
			stream.commit(header_size);

			int32_t bytes_to_read = 0;
			input_stream >> bytes_to_read;
			stream.prepare(bytes_to_read);
			auto body_bytes = co_await asio::async_read(socket, stream, use_awaitable);
			stream.commit(body_bytes);

			com::CommandReply reply{};
			if (reply.ParseFromIstream(&input_stream))
			{
				std::println("Received {0} bytes message body: \n{1}", body_bytes, reply.reply());
			}
			else
			{
				std::println("Failed to parse {0} byte message body!", body_bytes);
			}
		}
	}
	catch (std::exception&)
	{
		stop(socket);
	}
}

awaitable<void> connect(tcp::socket socket, char* addr, char* service)
{
	auto& context = socket.get_executor();
	std::error_code ec;
	tcp::resolver res(context);
	std::deque<std::string> commands;

	std::println("Connecting to {0}:{1}...", addr, service);
	
	auto resolved = co_await res.async_resolve(addr, service, asio::redirect_error(asio::use_awaitable, ec));
	co_await asio::async_connect(socket, resolved, asio::redirect_error(asio::use_awaitable, ec));

	co_spawn(context, reader(socket), detached);

	if (!ec)
		std::cout << "Connected to " << socket.remote_endpoint() << ".\n";

	constexpr std::size_t header_size = sizeof(int32_t);

	asio::streambuf buffer{};
	std::ostream output_stream(&buffer);

	while (socket.is_open())
	{
		std::cout << "Enter message: ";
		std::string str{};
		std::getline(std::cin, str);

		if (strcmp(str.c_str(), "exit") == 0)
		{
			socket.close();
			break;
		}

		com::CommandQuery query;
		query.set_command(str);

		int32_t query_size = static_cast<int32_t>(query.ByteSizeLong());
		output_stream.write(reinterpret_cast<char*>(&query_size), sizeof(query_size));
		query.SerializeToOstream(&output_stream);

		//int32_t num = static_cast<int32_t>(sizeof(query));
		//const char* buf_test = reinterpret_cast<const char*>(&num);

		//std::size_t n = co_await asio::async_write(socket, buffer, asio::redirect_error(use_awaitable, ec));
		std::size_t n = co_await asio::async_write(socket, buffer, asio::transfer_exactly(sizeof(query_size) + query_size), asio::redirect_error(use_awaitable, ec));
		std::println("Wrote {} bytes (buffer {}/{}) - {}", n, sizeof(query_size), query_size, ec.message());
		buffer.consume(n);
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