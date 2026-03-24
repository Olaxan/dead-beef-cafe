#include "dbc.h"

#include "msg_queue.h"
#include "task.h"
#include "world.h"
#include "host.h"
#include "os.h"
#include "os_basic.h"
#include "uid64.h"
#include "nic.h"
#include "net_mgr.h"

#include "CLI/CLI.hpp"

#include "proto/query.pb.h"
#include "proto/reply.pb.h"
#include "proto/ip_packet.pb.h"

#include <google/protobuf/util/delimited_message_util.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/detached.hpp>
#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read_until.hpp>
#include <asio/redirect_error.hpp>
#include <asio/signal_set.hpp>
#include <asio/steady_timer.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

#include <coroutine>
#include <string>
#include <vector>
#include <print>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::redirect_error;
using asio::use_awaitable;

using namespace google;

namespace protoutils = google::protobuf::util;



auto client_read_route(NetManager* net)
{
  	return asio::async_compose<decltype(asio::use_awaitable), void(ip::IpPackage)>(
    	[net](auto&& self) -> EagerTask<int32_t>
      	{
			auto self_ptr = std::make_shared<std::decay_t<decltype(self)>>(std::move(self));
			ip::IpPackage rep = co_await net->async_read_route();
			self_ptr->complete(rep);
			co_return 0;
      	},
      	asio::use_awaitable
    );
}


/* Shell session -- represents a connection between this (real + fake) client and a (real + fake) server. */
class ShellClient : public std::enable_shared_from_this<ShellClient>
{
public:

	ShellClient(Proc& proc, asio::io_context& context)
	: proc_(proc), context_(context), timer_(context), socket_(tcp::socket(context))
	{
		timer_.expires_at(std::chrono::steady_clock::time_point::max());
		net_mgr_ = proc_.owning_os->get_network_manager();
	}

	void connect(std::string addr, std::string service)
	{
		co_spawn(context_, 
			[self = shared_from_this(), addr, service] { return self->resolve(addr, service); }, detached);
	}

	void write(ip::IpPackage&& query)
	{
		asio::post(context_, [this, query] mutable
		{
			write_queue_.push(std::move(query));
			timer_.cancel_one();
		});
	}

	void deliver(ip::IpPackage&& msg)
	{
		net_mgr_->receive(std::move(msg));
		timer_.cancel_one();
	}

	awaitable<void> resolve(std::string addr, std::string service)
	{
		std::error_code ec;
		tcp::resolver res(context_);

		proc_.putln("Connecting to {0}:{1}...", addr, service);
		
		auto resolved = co_await res.async_resolve(addr, service, asio::redirect_error(asio::use_awaitable, ec));
		co_await asio::async_connect(socket_, resolved, asio::redirect_error(asio::use_awaitable, ec));

		if (!ec)
		{
			proc_.putln("Connected to {}.", socket_.remote_endpoint().address().to_string());
		}

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
					continue;
				}

				asio::streambuf stream{};
				std::istream input_stream(&stream);
				asio::streambuf::mutable_buffers_type buffer = stream.prepare(body_size);

				auto body_bytes = co_await asio::async_read(socket_, buffer, asio::transfer_exactly(body_size), use_awaitable);
				stream.commit(body_size);

				std::string received_str = DbcUtils::make_string(stream);

				ip::IpPackage pak;
				if (pak.ParseFromString(received_str))
				{
					deliver(std::move(pak));
				}
				else
				{
					proc_.errln("Failed to parse {0} byte message body!", body_bytes);
					std::size_t hash = std::hash<std::string>{}(received_str);
					proc_.putln("Received: '{}' ({}).", received_str, hash);
				}
			}
		}
		catch (const std::exception& e)
		{
			proc_.errln("join: Read exception: {}.", e.what());
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

				ip::IpPackage send = co_await client_read_route(net_mgr_);
	
				std::string coded_str;
				bool success = send.SerializeToString(&coded_str);

				int32_t query_size = static_cast<int32_t>(coded_str.size());
				int32_t header_size = static_cast<int32_t>(sizeof(query_size));
				int32_t total_msg_size = query_size + header_size;
				output_stream.write(reinterpret_cast<char*>(&query_size), sizeof(query_size));
				output_stream.write(reinterpret_cast<char*>(coded_str.data()), query_size);

				std::size_t n = co_await asio::async_write(socket_, buffer, asio::transfer_exactly(total_msg_size), asio::redirect_error(use_awaitable, ec));
				buffer.consume(n);
			}
		}
		catch(const std::exception& e)
		{
			proc_.errln("join: Write exception: {}.", e.what());
			stop();
		}
	}

	void stop()
	{
		socket_.close();
		timer_.cancel();
		context_.stop();
	}

	Proc& proc_;
	NetManager* net_mgr_{nullptr};
	asio::io_context& context_;
	asio::steady_timer timer_;
	tcp::socket socket_;
	NetQueue write_queue_{};

};


ProcessTask Programs::CmdDbcClient(Proc& proc, std::vector<std::string> args)
{

	CLI::App app{"An out-of-world utility for connecting to a DEAD:BEEF:CAFE:: routing host!"};
	app.allow_windows_style_options(false);

	struct DbcClientArgs
	{
		std::string addr{"localhost"};
		std::string service{"666"};
	} params{};

	app.add_option("-a,ADDR", params.addr, "Address for connection")->capture_default_str();
	app.add_option("-p,SERV", params.service, "Service or port for connection")->capture_default_str();

	try
	{
		std::ranges::reverse(args);
		args.pop_back();
        app.parse(std::move(args));
    }
	catch(const CLI::ParseError& e)
	{
		int res = app.exit(e, proc.s_out, proc.s_err);
        co_return res;
    }

	asio::io_context io_context(1);

	proc.putln("Starting routing agent...");

	try
	{
		auto client = std::make_shared<ShellClient>(proc, io_context);
		client->connect(params.addr, params.service);

		asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&](auto, auto) { io_context.stop(); });

		co_await IoServiceAwaiter{io_context};
	}
	catch (const std::exception& e)
	{
		proc.errln("join: Exception: {}.", e.what());
	}

	proc.putln("join: DBC link exited.");

    co_return 0;
}