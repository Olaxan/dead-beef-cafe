#include "proc_ioapi.h"

#include "proto/query.pb.h"

#include "proc.h"
#include "proc_types.h"
#include "net_types.h"

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

	icu::UnicodeString buffer;

	while (true)
	{
		auto exp_query = co_await read_query();

		if (not exp_query)
		{
			proc.errln("Failed to establish reader.");
			co_return std::unexpected{exp_query.error()};
		}

		const com::CommandQuery& query = *exp_query;

		if (callback)
		{
			std::invoke(callback, query);
		}

		/* Next, read the actual command and consider it based on first-byte. */
		std::string str_in = query.command();

		if (str_in.length() == 0)
			continue;

		if (str_in[0] == '\x1b')
			continue;

		if (str_in[0] == '\t')
			continue;

		if (str_in[0] == '\r' || str_in[0] == '\n')
		{
			proc.put("\r\n");
			break;
		}

		if (str_in[0] == '\x08' || str_in[0] == '\x7f')
		{
			UErrorCode status = U_ZERO_ERROR;
			std::unique_ptr<icu::BreakIterator> bi(icu::BreakIterator::createCharacterInstance(icu::Locale::getDefault(), status));
			if (U_FAILURE(status) || !bi)
			{
				proc.warnln("Warning: Failed to create break iterator!");
				continue;
			}

			bi->setText(buffer);
			int32_t end = buffer.length();
			int32_t last_char_start = bi->preceding(end);
			if (last_char_start != icu::BreakIterator::DONE && params.echo)
			{
				int32_t points_removed = end - last_char_start;
				buffer.remove(last_char_start);
				proc.put(CSI "{}D" CSI "0K", points_removed);
			}

			continue;
		}

		if (str_in.back() == '\0')
			str_in.pop_back();

		icu::UnicodeString chunk = icu::UnicodeString::fromUTF8(str_in);
		buffer.append(chunk);
		
		if (params.echo)
		{
			std::string writeback;
			icu::UnicodeString ret_str = chunk.unescape();
			ret_str.toUTF8String(writeback);
			proc.put("{}", params.password ? std::string(ret_str.length(), '*') : writeback);
		}
	}

	/* Trim any whitespace from the buffer string, convert the result back to utf8,
	and clear the buffer before sending it for processing. */
	std::string out_cmd;
	buffer.trim();
	buffer.toUTF8String(out_cmd);
	buffer.remove();

	co_return out_cmd;
}