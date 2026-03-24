#pragma once

#include "task.h"
#include "net_types.h"
#include "proc_types.h"

#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <string>

class Proc;

using CmdQueryFn = std::function<void(const com::CommandQuery&)>;
using ReadResultQuery = std::expected<com::CommandQuery, std::error_condition>;
using ReadResultReply = std::expected<com::CommandReply, std::error_condition>;

struct CmdReaderParams
{
	bool echo{true};
	bool password{false};
};

class ProcIoApi
{
public:

	ProcIoApi() = delete;
	ProcIoApi(Proc* owner);
	~ProcIoApi();

	EagerTask<ReadResultQuery> read_query();
	EagerTask<ReadResultReply> read_reply();
	void write_query(const com::CommandQuery& query);
	void write_reply(const com::CommandReply& reply);
	EagerTask<ReadResult> read_cmd_utf8(CmdReaderParams params, CmdQueryFn callback = nullptr);

protected:

	Proc& proc;
	
};