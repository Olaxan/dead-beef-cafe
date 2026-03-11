#pragma once

#include "task.h"
#include "net_types.h"

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
	void write_query(Proc& proc, const com::CommandQuery& query);
	void write_reply(Proc& proc, const com::CommandReply& reply);
	EagerTask<std::string> read_cmd_utf8(Proc& proc, CmdReaderParams params, CmdQueryFn callback = nullptr);
}