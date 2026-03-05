#pragma once

#include "task.h"
#include "netw.h"

#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <functional>
#include <string>

class Proc;

using CmdQueryFn = std::function<void(const com::CommandQuery&)>;

namespace CmdInput
{
	struct CmdReaderParams
	{
		bool echo{true};
		bool password{false};
	};

	EagerTask<com::CommandQuery> read_query(Proc& proc);
	EagerTask<com::CommandReply> read_reply(Proc& proc);
	EagerTask<std::string> read_cmd_utf8(Proc& proc, CmdReaderParams params, CmdQueryFn callback = nullptr);
}