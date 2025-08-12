#pragma once

#include "task.h"
#include "netw.h"

#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <functional>
#include <string>

class Proc;

using CmdSocketServer = Socket<com::CommandQuery, com::CommandReply>;
using CmdSocketClient = Socket<com::CommandReply, com::CommandQuery>;
using CmdSocketAwaiterServer = MessageAwaiter<com::CommandQuery, com::CommandReply>;
using CmdSocketAwaiterClient = MessageAwaiter<com::CommandReply, com::CommandQuery>;
using CmdQueryFn = std::function<void(const com::CommandQuery&)>;

namespace CmdInput
{
	EagerTask<std::string> read_cmd_utf8(Proc& proc, CmdQueryFn callback = nullptr);
}