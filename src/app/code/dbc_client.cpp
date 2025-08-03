#include "msg_queue.h"

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

#include <windows.h>

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

std::string make_string(asio::streambuf& streambuf)
{
	return { asio::buffers_begin(streambuf.data()), asio::buffers_end(streambuf.data()) };
}

void deliver(com::CommandReply reply)
{
	std::print("{}", reply.reply());
}

class ShellClient : public std::enable_shared_from_this<ShellClient>
{
public:

	ShellClient(asio::io_context& context)
	: context_(context), timer_(context), socket_(tcp::socket(context))
	{
		timer_.expires_at(std::chrono::steady_clock::time_point::max());
	}

	void connect(char* addr, char* service)
	{
		co_spawn(context_, 
			[self = shared_from_this(), addr, service] { return self->resolve(addr, service); }, detached);
	}

	void write(com::CommandQuery&& query)
	{
		asio::post(context_, [this, query] mutable
		{
			write_queue_.push(std::move(query));
			timer_.cancel_one();
		});
	}

	awaitable<void> resolve(char* addr, char* service)
	{
		std::error_code ec;
		tcp::resolver res(context_);

		std::println("Connecting to {0}:{1}...", addr, service);
		
		auto resolved = co_await res.async_resolve(addr, service, asio::redirect_error(asio::use_awaitable, ec));
		co_await asio::async_connect(socket_, resolved, asio::redirect_error(asio::use_awaitable, ec));

		if (!ec)
			std::cout << "Connected to " << socket_.remote_endpoint() << ".\n";

		co_spawn(context_, 
			[self = shared_from_this()] { return self->reader(); }, detached);
		co_spawn(context_, 
			[self = shared_from_this()] { return self->writer(); }, detached);
	}

	awaitable<void> reader()
	{
		constexpr std::size_t header_size = sizeof(int32_t);

		try
		{
			while (socket_.is_open())
			{
				int32_t body_size = co_await std::invoke([this] -> awaitable<int32_t>
				{
					asio::streambuf stream{};
					std::istream input_stream(&stream);
					asio::streambuf::mutable_buffers_type buffer = stream.prepare(header_size);

					auto header_bytes = co_await asio::async_read(socket_, buffer, asio::transfer_exactly(header_size), use_awaitable);
					stream.commit(header_size);

					int32_t bytes_to_read = 0;
					input_stream.read(reinterpret_cast<char*>(&bytes_to_read), sizeof(bytes_to_read));
					co_return bytes_to_read;
				});

				if (body_size == 0)
				{
					std::println("Warning: body size of 0 bytes -- closing.");	
					stop();
					break;
				}

				asio::streambuf stream{};
				std::istream input_stream(&stream);
				asio::streambuf::mutable_buffers_type buffer = stream.prepare(body_size);

				auto body_bytes = co_await asio::async_read(socket_, buffer, asio::transfer_exactly(body_size), use_awaitable);
				stream.commit(body_size);

				std::string received_str = make_string(stream);

				com::CommandReply reply;
				//if (query.ParseFromIstream(&input_stream))
				if (reply.ParseFromString(received_str))
				{
					deliver(std::move(reply));
				}
				else
				{
					std::println("Failed to parse {0} byte message body!", body_bytes);
					std::size_t hash = std::hash<std::string>{}(received_str);
					std::println("Received: '{}' ({}).", received_str, hash);
				}
			}
		}
		catch (std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << "\n";
			stop();
		}
	}

	awaitable<void> writer()
	{
		try
		{
			while (socket_.is_open())
			{
				asio::error_code ec;
				asio::streambuf buffer{};
				std::ostream output_stream(&buffer);
	
				if (std::optional<com::CommandQuery> opt_query = write_queue_.pop(); opt_query.has_value())
				{
					com::CommandQuery query = opt_query.value();
	
					std::string coded_str;
					bool success = query.SerializeToString(&coded_str);
					//std::println("Serialised input '{}' to '{}' ({}); success = {}.", str, coded_str, hash, success);
	
					//int32_t query_size = static_cast<int32_t>(query.ByteSizeLong());
					int32_t query_size = static_cast<int32_t>(coded_str.size());
					int32_t header_size = static_cast<int32_t>(sizeof(query_size));
					int32_t total_msg_size = query_size + header_size;
					output_stream.write(reinterpret_cast<char*>(&query_size), sizeof(query_size));
					output_stream.write(reinterpret_cast<char*>(coded_str.data()), query_size);
					//query.SerializeToOstream(&output_stream);
	
					std::size_t n = co_await asio::async_write(socket_, buffer, asio::transfer_exactly(total_msg_size), asio::redirect_error(use_awaitable, ec));
					buffer.consume(n);
				}
				else
				{
					co_await timer_.async_wait(redirect_error(use_awaitable, ec));
				}
			}
		}
		catch(const std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << "\n";
			stop();
		}
	}

	void stop()
	{
		google::protobuf::ShutdownProtobufLibrary();
		socket_.close();
		timer_.cancel();
	}

	asio::io_context& context_;
	asio::steady_timer timer_;
	tcp::socket socket_;
	MessageQueue<com::CommandQuery> write_queue_{};

};

bool EnableVTMode()
{
    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
    {
        return false;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode))
    {
        return false;
    }

    return true;
}

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
	try
	{

		if (argc != 3)
		{
			std::cerr << "Usage: dbc_client <host> <port>\n";
			return 1;
		}

		asio::io_context io_context(1);

		auto client = std::make_shared<ShellClient>(io_context);
		client->connect(argv[1], argv[2]);

		asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&](auto, auto) { io_context.stop(); });
		
		std::jthread runner([&io_context] { io_context.run(); });

		while (true)
		{
			std::string str{};
			std::getline(std::cin, str);

			if (strcmp(str.c_str(), "exit") == 0)
			{
				break;
			}

			com::CommandQuery query;
			query.set_command(str);

			com::ScreenData* screen = get_screen_data();

			query.set_allocated_screen_data(screen);
			client->write(std::move(query));
		}

		client->stop();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}