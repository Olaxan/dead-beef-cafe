#pragma once

#include "task.h"
#include "net_types.h"
#include "proc_types.h"
#include "input_field.h"

#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <string>

class Proc;

using CmdReadEvent = InputField::HandlerReturn;
using CmdEventResponse = InputField::EventFilterResponse;

using CmdFilterFn = std::function<CmdEventResponse(InputField&, const com::CommandQuery&, CmdReadEvent)>;
using ReadResultQuery = std::expected<com::CommandQuery, std::error_condition>;
using ReadResultReply = std::expected<com::CommandReply, std::error_condition>;

struct CmdReaderParams
{
	bool echo{true};
	bool password{false};
	CmdFilterFn filter{nullptr};
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
	EagerTask<ReadResult> read_cmd_utf8(CmdReaderParams params);

protected:

	Proc& proc;
	
};