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
#include "host_utils.h"

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
    	[ptr](auto&& self)
      	{
			auto self_ptr = std::make_shared<std::decay_t<decltype(self)>>(std::move(self));

			MessageQueue<com::CommandReply>& rxq = ptr->rxq;
			if (auto opt = rxq.pop(); opt.has_value())
			{
				self_ptr->complete(opt.value());
			}
			else
			{
				ptr->add_await_rx([self_ptr](const com::CommandReply& msg) mutable
				{
					self_ptr->complete(msg);	
				});
			}
      	},
      	asio::use_awaitable
    );
}

auto read_sock2(const std::shared_ptr<CmdSocketClient>& ptr)
{
  	return asio::async_compose<decltype(asio::use_awaitable), void(com::CommandReply)>(
    	[ptr](auto&& self) -> EagerTask<int32_t>
      	{
			auto self_ptr = std::make_shared<std::decay_t<decltype(self)>>(std::move(self));
			com::CommandReply rep = co_await ptr->read_one();
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
		timer_.expires_at(std::chrono::steady_clock::time_point::max());
	}

	void start()
	{
		std::cout << "Client joined from " << socket_.remote_endpoint() << ".\n";
		
		OS& local_os = local_->get_os();
		shell_ = local_os.get_shell(out_stream_);
		sock_ = local_os.create_socket<CmdSocketClient>();
		local_os.connect_socket<CmdSocketServer>(sock_, Address6(1), 22);

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
		sock_->write_one(std::move(msg));
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

					std::println("Awaiting to read {0} bytes...", header_size);
					auto header_bytes = co_await asio::async_read(socket_, buffer, asio::transfer_exactly(header_size), use_awaitable);
					std::println("Got header: {0} bytes ({1} expected).", header_bytes, header_size);
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

				std::println("Awaiting to read body ({0} bytes)...", body_size);
				auto body_bytes = co_await asio::async_read(socket_, buffer, asio::transfer_exactly(body_size), use_awaitable);
				std::println("Read {0} bytes ({1} expected).", body_bytes, body_size);
				stream.commit(body_size);

				std::string received_str = make_string(stream);

				com::CommandQuery query;
				//if (query.ParseFromIstream(&input_stream))
				if (query.ParseFromString(received_str))
				{
					std::println("Received {0} bytes message body: \n{1}", body_bytes, query.command());
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
		catch (std::exception&)
		{
			stop();
		}
 	}

	awaitable<void> writer()
	{
		try
		{
			constexpr std::size_t header_size = sizeof(int32_t);

			asio::streambuf stream{};
			std::ostream output_stream(&stream);

			while (socket_.is_open())
			{
				//com::CommandReply reply = co_await sock_->read_one();
				com::CommandReply reply = co_await read_sock2(sock_);

				std::println("Reply: {0}", reply.reply());

				// com::CommandReply reply{};
				// reply.ParseFromIstream(&in_stream_);
				// reply.set_reply(write_msgs_.front());
				// write_msgs_.pop_front();

				// output_stream << static_cast<int32_t>(sizeof(reply));
				// auto header_bytes_sent = co_await asio::async_write(socket_, stream.data(), use_awaitable);
				// reply.SerializeToOstream(&output_stream);
				// auto body_bytes_sent = co_await asio::async_write(socket_, stream.data(), use_awaitable);

				// std::println("Sent {0} bytes ({1} header, {2} body).", 
				// (header_bytes_sent + body_bytes_sent), header_bytes_sent, body_bytes_sent);
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

	Proc* shell_;
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
		if (argc < 2)
		{
			std::cerr << "Usage: dbc_server <port> [<port> ...]\n";
			return 1;
		}

		asio::io_context io_context(1);

		for (int i = 1; i < argc; ++i)
		{
			unsigned short port = std::atoi(argv[i]);
			co_spawn(io_context,
				listener(tcp::acceptor(io_context, {tcp::v4(), port})),
				detached);
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