#include "os_input.h"

#include "proto/query.pb.h"

#include "proc.h"
#include "net_types.h"

#include <print>
#include <optional>

EagerTask<com::CommandQuery> CmdInput::read_query(Proc& proc)
{
	while (true)
	{
		std::string str = co_await proc.read();
	
		com::CommandQuery query;
		if (query.ParseFromString(str))
			co_return query;
	}

	co_return {};
}

EagerTask<com::CommandReply> CmdInput::read_reply(Proc& proc)
{
	while (true)
	{
		std::string str = co_await proc.read();
	
		com::CommandReply reply;
		if (reply.ParseFromString(str))
			co_return reply;
	}

	co_return {};
}

void CmdInput::write_query(Proc& proc, const com::CommandQuery& query)
{
	std::string out;
	query.SerializeToString(&out);
	proc.write(out);
}

void CmdInput::write_reply(Proc& proc, const com::CommandReply& reply)
{
	std::string out;
	reply.SerializeToString(&out);
	proc.write(out);
}

EagerTask<std::string> CmdInput::read_cmd_utf8(Proc& proc, CmdReaderParams params, CmdQueryFn callback)
{

	icu::UnicodeString buffer;

	while (true)
	{
		std::optional<com::CommandQuery> query = co_await read_query(proc);

		if (!query)
		{
			proc.errln("Failed to establish reader.");
			co_return {};
		}

		if (callback)
		{
			std::invoke(callback, *query);
		}

		/* Next, read the actual command and consider it based on first-byte. */
		std::string str_in = query->command();

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