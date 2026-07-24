#include "proc_ioapi.h"

#include "proto/query.pb.h"

#include "proc.h"
#include "proc_types.h"
#include "net_types.h"
#include "input_field.h"

#include <print>
#include <optional>

#include <iso646.h>

ProcIoApi::ProcIoApi(Proc* owner)
: proc(*owner) { }

ProcIoApi::~ProcIoApi() = default;

EagerTask<ReadResultQuery> ProcIoApi::read_query()
{
	while (true)
	{
		ReadResult str = co_await proc.read();
		if (not str)
			co_return std::unexpected{str.error()};
	
		com::CommandQuery query;
		if (not query.ParseFromString(*str))
			co_return std::unexpected{std::error_condition{EIO, std::generic_category()}};
		
		co_return query;
	}
}

EagerTask<ReadResultReply> ProcIoApi::read_reply()
{
	while (true)
	{
		ReadResult str = co_await proc.read();
		if (not str)
			co_return std::unexpected{str.error()};
	
		com::CommandReply reply;
		if (not reply.ParseFromString(*str))
			co_return std::unexpected{std::error_condition{EIO, std::generic_category()}};
		
		co_return reply;
	}
}

void ProcIoApi::write_query(const com::CommandQuery& query)
{
	std::string out;
	query.SerializeToString(&out);
	proc.write(out);
}

void ProcIoApi::write_reply(const com::CommandReply& reply)
{
	std::string out;
	reply.SerializeToString(&out);
	proc.write(out);
}

EagerTask<ReadResult> ProcIoApi::read_cmd_utf8(CmdReaderParams params, CmdQueryFn callback)
{

	InputFieldParams field_params
	{
		.multiline = false
	};

	InputField field{std::move(field_params)};
	bool run{true};

	proc.write(SAVE_CURSOR);

	while (run)
	{
		auto exp_query = co_await read_query();

		if (not exp_query)
		{
			co_return std::unexpected{exp_query.error()};
		}

		const com::CommandQuery& query = *exp_query;
		std::string str_in = query.command();

		if (callback)
		{
			std::invoke(callback, query);
		}

		auto input_callback = [&](std::string_view input, InputField::HandlerReturn event)
		{
			if (event == InputField::HandlerReturn::Return)
			{
				run = false;
				return InputField::EventFilterResponse::Handled;
			}

			if (event == InputField::HandlerReturn::Tab)
			{
				return InputField::EventFilterResponse::Handled;
			}

			return InputField::EventFilterResponse::Unhandled;
		};

		InputField::EventResponse ret = field.feed(str_in, input_callback);
		
		if (params.echo)
		{
			std::string writeback = field.render_line_utf8(true);
			int32_t line_len = field.render_line_length();
			int32_t cursor_x = field.get_adjusted_col();

			std::stringstream ss{};
			ss << RESTORE_CURSOR;
			ss << ERASE_FROM;
			ss << (params.password ? std::string(line_len, '*') : writeback);
			ss << RESTORE_CURSOR;

			if (cursor_x > 0)
			{
				ss << CSI << cursor_x << "C";
			}

			proc.write(ss.str());
		}
	}

	proc.write("\n");

	co_return field.as_utf8();
}