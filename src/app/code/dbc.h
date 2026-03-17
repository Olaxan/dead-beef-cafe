#pragma once

#include "os.h"
#include "proc.h"

#include <coroutine>
#include <vector>
#include <string>

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