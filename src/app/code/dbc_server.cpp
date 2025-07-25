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

#include "proto/test.pb.h"
#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
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

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::redirect_error;
using asio::use_awaitable;

//----------------------------------------------------------------------

class ChatParticipant
{
public:
	virtual ~ChatParticipant() {}
	virtual void deliver(const std::string& msg) = 0;
};

typedef std::shared_ptr<ChatParticipant> ChatParticipantPtr;

//----------------------------------------------------------------------

class ChatRoom
{
public:

	ChatRoom()
	{
		server_host_ = world_.create_host<BasicOS>("Server");
		server_host_->start_host();
		world_.launch();
	}

	void join(ChatParticipantPtr participant)
	{
		participants_.insert(participant);
		for (auto msg: recent_msgs_)
			participant->deliver(msg);
	}

	void leave(ChatParticipantPtr participant)
	{
		participants_.erase(participant);
	}

	void deliver(const std::string& msg)
	{
		auto& queue = world_.get_update_queue();

		//queue.push([this, msg]{ local_os_.exec(msg); });

		recent_msgs_.push_back(msg);
		while (recent_msgs_.size() > max_recent_msgs)
			recent_msgs_.pop_front();

		for (auto participant: participants_)
			participant->deliver(msg);
	}

	World& get_world() { return world_; }

private:

	World world_{};
	Host* server_host_{nullptr};
	std::set<ChatParticipantPtr> participants_;
	enum { max_recent_msgs = 100 };
	std::deque<std::string> recent_msgs_;

};

//----------------------------------------------------------------------

class ShellSession : public ChatParticipant, public std::enable_shared_from_this<ShellSession>
{
public:

	ShellSession(tcp::socket socket, ChatRoom& room)
	: socket_(std::move(socket)), timer_(socket_.get_executor()), room_(room), world_(room_.get_world())
	{
		local_ = world_.create_host<BasicOS>("Participant");
		timer_.expires_at(std::chrono::steady_clock::time_point::max());
	}

	void start()
	{
		std::cout << "Client joined from " << socket_.remote_endpoint() << ".\n";
		
		OS& local_os = local_->get_os();
		shell_ = local_os.get_shell(out_stream_);
		sock_ = local_os.create_socket<com::CommandQuery>(22);

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

			asio::streambuf stream{};
			std::istream input_stream(&stream);

			while (true)
			{
				stream.prepare(header_size);
				auto header_bytes = co_await asio::async_read(socket_, stream, use_awaitable);
				std::println("Got header: {0} bytes ({1} expected).", header_bytes, header_size);
				stream.commit(header_size);

				int32_t bytes_to_read = 0;
				input_stream >> bytes_to_read;
				stream.prepare(bytes_to_read);
				std::println("Preparing to read body: {0} bytes.", bytes_to_read);
				auto body_bytes = co_await asio::async_read(socket_, stream, use_awaitable);
				std::println("Read {0} bytes ({1} expected).", body_bytes, bytes_to_read);
				stream.commit(body_bytes);

				com::CommandQuery query{};
				if (query.ParseFromIstream(&input_stream))
				{
					std::println("Received {0} bytes message body: \n{1}", body_bytes, query.command());
					deliver(std::move(query));
				}
				else
				{
					std::println("Failed to parse {0} byte message body!", body_bytes);
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
				if (write_msgs_.empty())
				{
					asio::error_code ec;
					co_await timer_.async_wait(redirect_error(use_awaitable, ec));
				}
				else
				{

					com::CommandReply reply{};
					reply.ParseFromIstream(&in_stream_);
					reply.set_reply(write_msgs_.front());
					write_msgs_.pop_front();

					output_stream << static_cast<int32_t>(sizeof(reply));
					auto header_bytes_sent = co_await asio::async_write(socket_, stream.data(), use_awaitable);
					reply.SerializeToOstream(&output_stream);
					auto body_bytes_sent = co_await asio::async_write(socket_, stream.data(), use_awaitable);

					std::println("Sent {0} bytes ({1} header, {2} body).", 
						(header_bytes_sent + body_bytes_sent), header_bytes_sent, body_bytes_sent);
				}
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

	std::deque<std::string> write_msgs_{};

	Proc* shell_;
	Socket<com::CommandQuery>* sock_{nullptr};

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