#include "dbc.h"

#include "msg_queue.h"
#include "task.h"
#include "world.h"
#include "host.h"
#include "os.h"
#include "os_basic.h"
#include "os_input.h"
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

class DbcParticipant
{
public:
	virtual ~DbcParticipant() = default;
	virtual void deliver(ip::IpPackage&& msg) = 0;
};

typedef std::shared_ptr<DbcParticipant> DbcParticipantPtr;


/* This nasty function converts from a DBC task to an ASIO awaitable. */
auto read_route(NetManager* net)
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


/* Shell session -- represents a connection between a (real + fake) client and this (real + fake) server. */
class ShellSession : public DbcParticipant, public std::enable_shared_from_this<ShellSession>
{
public:

	ShellSession(Proc& proc, tcp::socket socket)
	: proc_(proc), socket_(std::move(socket)), timer_(socket_.get_executor())
	{
		timer_.expires_at(std::chrono::steady_clock::time_point::max());
		local_nic_ = proc.owning_os->get_device<NIC>();
		net_mgr_ = proc.owning_os->get_network_manager();
		assert(local_nic_);
	}

	~ShellSession() = default;

	void start()
	{
		proc_.putln("Client joined from {}.", socket_.remote_endpoint().address().to_string());
		
		co_spawn(socket_.get_executor(),
			[self = shared_from_this()]{ return self->reader(); },
			detached);

		co_spawn(socket_.get_executor(),
			[self = shared_from_this()]{ return self->writer(); },
			detached);
	}

	void deliver(ip::IpPackage&& msg)
	{
		net_mgr_->safe_rx(std::move(msg));
		timer_.cancel_one();
	}

private:

	awaitable<void> reader()
	{
		try
		{
			constexpr std::size_t header_size = sizeof(int32_t);

			while (true)
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
					std::println("Warning: body size of 0 bytes.");	
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
			proc_.errln("host: Read exception: {}.", e.what());
			stop();
		}
 	}

	awaitable<void> writer()
	{
		try
		{
			constexpr std::size_t header_size = sizeof(int32_t);

			std::error_code ec;
			asio::streambuf buffer{};
			std::ostream output_stream(&buffer);

			while (socket_.is_open())
			{
				ip::IpPackage reply = co_await read_route(net_mgr_);

				std::string coded_str;
				bool success = reply.SerializeToString(&coded_str);
				int32_t reply_size = static_cast<int32_t>(coded_str.size());
				int32_t header_size = static_cast<int32_t>(sizeof(reply_size));
				int32_t total_msg_size = reply_size + header_size;
				output_stream.write(reinterpret_cast<char*>(&reply_size), sizeof(reply_size));
				output_stream.write(reinterpret_cast<char*>(coded_str.data()), reply_size);

				std::size_t n = co_await asio::async_write(socket_, buffer, asio::transfer_exactly(total_msg_size), asio::redirect_error(use_awaitable, ec));
				buffer.consume(n);

				if (static_cast<int32_t>(n) != total_msg_size)
					proc_.errln("Write mismatch: wrote {0} bytes but expected {1}.", n, total_msg_size);
			}
		}
		catch (std::exception& e)
		{
			proc_.errln("host: Write exception: {}.", e.what());
			stop();
		}
  	}

	void stop()
	{
		socket_.close();
		timer_.cancel();
	}

	Proc& proc_;
	tcp::socket socket_;
	asio::steady_timer timer_;
	NIC* local_nic_{nullptr};
	NetManager* net_mgr_{nullptr};

	asio::streambuf in_buf_{};
	std::istream in_stream_{&in_buf_};

	asio::streambuf out_buf_{};
	std::ostream out_stream_{&out_buf_};

};


/* Listener, sets up a shell session for every joining client! */
awaitable<void> listener(Proc& proc, tcp::acceptor acceptor)
{
	for (;;)
	{
		auto ptr = std::make_shared<ShellSession>(proc, co_await acceptor.async_accept(use_awaitable));
		ptr->start();
	}
}

/* This program can be run on a node in the host network to set it up as a 
real listening server in the fake internet, allowing users to connect and route traffic through it. */
ProcessTask Programs::CmdDbcServer(Proc& proc, std::vector<std::string> args)
{

	CLI::App app{"An out-of-world utility for hosting a DEAD:BEEF:CAFE:: routing host!"};
	app.allow_windows_style_options(false);

	struct DbcServerArgs
	{
		std::vector<unsigned short> ports{666};
	} params{};

	app.add_option("-p,PORTS", params.ports, "Ports upon which to listen for joining clients")->capture_default_str();

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

	proc.putln("Hosting DBC server on {}...", params.ports);
	co_await proc.wait(1.f);

	asio::io_context io_context(1);

	try
	{
		for (unsigned short port : params.ports)
		{
			co_spawn(io_context, listener(proc, tcp::acceptor(io_context, {tcp::v4(), port})), detached);
		}

		asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&](auto, auto){ io_context.stop(); });

		co_await IoServiceAwaiter{io_context};
	}
	catch (const std::exception& e)
	{
		proc.errln("join: Exception: {}.", e.what());
	}

	co_return 0;
}