#pragma once

#include "os.h"
#include "proc.h"

#include <coroutine>
#include <vector>
#include <string>

#include <asio/io_context.hpp>
#include <asio/buffer.hpp>
#include <asio/streambuf.hpp>

#include <iso646.h>


namespace DbcUtils
{
	/* Make a string from a ASIO buffer containing data. */
	std::string make_string(asio::streambuf& streambuf);
};

namespace Programs
{
	ProcessTask CmdDbcClient(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdDbcServer(Proc& proc, std::vector<std::string> args);
};

struct IoServiceAwaiter
{
	explicit IoServiceAwaiter(asio::io_context& srv)
		: srv_(srv) { }

	IoServiceAwaiter(IoServiceAwaiter&) = delete;

	bool await_ready() const { return false; }
	void await_suspend(std::coroutine_handle<> h);
	auto await_resume() const { return count_; }

private:

	asio::io_context& srv_;
	asio::io_context::count_type count_;

};