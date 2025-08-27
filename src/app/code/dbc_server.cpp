//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "world.h"
#include "host.h"
#include "os.h"
#include "msg_queue.h"
#include "os_basic.h"
#include "os_input.h"
#include "host_utils.h"
#include "uuid.h"

#include "proto/test.pb.h"
#include "proto/query.pb.h"
#include "proto/reply.pb.h"

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

//----------------------------------------------------------------------

class ChatParticipant
{
public:
	virtual ~ChatParticipant() {}
	virtual void deliver(com::CommandQuery&& msg) = 0;
};

typedef std::shared_ptr<ChatParticipant> ChatParticipantPtr;

//----------------------------------------------------------------------

class ChatRoom
{
public:

	ChatRoom()
	{
		server_host_ = HostUtils::create_host<BasicOS>(world_, "Server");
		server_host_->start_host();
		world_.launch();
	}

	void join(ChatParticipantPtr participant)
	{
		participants_.insert(participant);
	}

	void leave(ChatParticipantPtr participant)
	{
		participants_.erase(participant);
	}

	void deliver(const std::string& msg)
	{
		auto& queue = world_.get_update_queue();

		//queue.push([this, msg]{ local_os_.exec(msg); });

	}

	World& get_world() { return world_; }
	Host* get_server() { return server_host_; }

private:

	World world_{};
	Host* server_host_{nullptr};
	std::set<ChatParticipantPtr> participants_;
	enum { max_recent_msgs = 100 };

};

//----------------------------------------------------------------------

std::string make_string(asio::streambuf& streambuf)
{
	return { asio::buffers_begin(streambuf.data()), asio::buffers_end(streambuf.data()) };
}

auto read_sock(const std::shared_ptr<CmdSocketClient>& ptr)
{
  	return asio::async_compose<decltype(asio::use_awaitable), void(com::CommandReply)>(
    	[ptr](auto&& self) -> EagerTask<int32_t>
      	{
			auto self_ptr = std::make_shared<std::decay_t<decltype(self)>>(std::move(self));
			com::CommandReply rep = co_await ptr->async_read();
			self_ptr->complete(rep);
			co_return 0;
      	},
      	asio::use_awaitable
    );
}

class ShellSession : public ChatParticipant, public std::enable_shared_from_this<ShellSession>
{
public:

	ShellSession(tcp::socket socket, ChatRoom& room)
	: socket_(std::move(socket)), timer_(socket_.get_executor()), room_(room), world_(room_.get_world())
	{
		local_ = HostUtils::create_host<BasicOS>(world_, "Participant");
		local_->start_host();
		timer_.expires_at(std::chrono::steady_clock::time_point::max());

		Host* remote = room_.get_server();
		HostLinkServer& links = world_.get_link_server();
		links.link(local_, remote);
		std::println("Linked {} to {}.", UUID{(uint64_t)local_}, UUID{(uint64_t)remote});
	}

	~ShellSession()
	{
		HostLinkServer& links = world_.get_link_server();
		links.purge(local_);
	}

	void start()
	{
		std::cout << "Client joined from " << socket_.remote_endpoint() << ".\n";
		
		OS& local_os = local_->get_os();
		local_os.run_process(Programs::CmdSSH, {"ssh"});
		sock_ = local_os.create_socket<CmdSocketClient>();
		local_os.connect_socket<CmdSocketServer>(sock_, local_os.get_global_ip(), 22);

		room_.join(shared_from_this());

		co_spawn(socket_.get_executor(),
			[self = shared_from_this()]{ return self->reader(); },
			detached);

		co_spawn(socket_.get_executor(),
			[self = shared_from_this()]{ return self->writer(); },
			detached);
	}

	void deliver(com::CommandQuery&& msg)
	{
		//write_msgs_.push_back(msg);
		sock_->write(std::move(msg));
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

				std::string received_str = make_string(stream);

				com::CommandQuery query;
				if (query.ParseFromString(received_str))
				{
					deliver(std::move(query));
				}
				else
				{
					std::println("Failed to parse {0} byte message body!", body_bytes);
					std::size_t hash = std::hash<std::string>{}(received_str);
					std::println("Received: '{}' ({}).", received_str, hash);
				}
			}
		}
		catch (const std::exception&)
		{
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

			while (socket_.is_open() && sock_->is_open())
			{
				com::CommandReply reply = co_await read_sock(sock_);

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
					std::println("Write mismatch: wrote {0} bytes but expected {1}.", n, total_msg_size);
			}
		}
		catch (std::exception&)
		{
			stop();
		}
  	}

	void stop()
	{
		google::protobuf::ShutdownProtobufLibrary();
		room_.leave(shared_from_this());
		socket_.close();
		timer_.cancel();
	}

	tcp::socket socket_;
	asio::steady_timer timer_;
	ChatRoom& room_;
	World& world_;
	Host* local_{nullptr};

	std::deque<com::CommandReply> replies_{};

	std::shared_ptr<CmdSocketClient> sock_{nullptr};

	asio::streambuf in_buf_{};
	std::istream in_stream_{&in_buf_};

	asio::streambuf out_buf_{};
	std::ostream out_stream_{&out_buf_};

};

//----------------------------------------------------------------------

awaitable<void> listener(tcp::acceptor acceptor)
{
	ChatRoom room;

	for (;;)
	{
		auto ptr = std::make_shared<ShellSession>(co_await acceptor.async_accept(use_awaitable), room);
		ptr->start();
	}
}

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
	try
	{
		std::vector<unsigned short> ports{};
		
		if (argc < 2)
		{
			std::println("Running on default port (666).");
			ports.push_back(666);
		}
		else
		{
			for (int i = 1; i < argc; ++i)
			{
				ports.push_back(std::atoi(argv[i]));
			}
		}

		asio::io_context io_context(1);

		for (unsigned short port : ports)
		{
			co_spawn(io_context, listener(tcp::acceptor(io_context, {tcp::v4(), port})), detached);
		}

		asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&](auto, auto){ io_context.stop(); });

		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}