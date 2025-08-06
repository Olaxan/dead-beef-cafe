#include "os_basic.h"

#include "filesystem.h"
#include "navigator.h"

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/usearch.h>
#include <unicode/ustring.h>
#include <unicode/ustream.h>
#include <unicode/brkiter.h>

#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>

ProcessTask Programs::CmdEdit(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	int32_t width = proc.get_var<int32_t>("TERM_W");
	int32_t height = proc.get_var<int32_t>("TERM_H");

	if (width < 20 || height < 10)
	{
		proc.warnln("The terminal window is too small, or 'TERM_W'/'TERM_H' wasn't set.");
		co_return 1;
	}

	com::CommandReply begin_msg;
	begin_msg.set_con_mode(com::ConsoleMode::Raw);
	begin_msg.set_configure(true);
	begin_msg.set_reply(
		BEGIN_ALT_SCREEN_BUFFER 
		CLEAR_CURSOR 
		CLEAR_SCREEN
		MOVE_CURSOR(2, 2));

	proc.write(begin_msg);

	std::string filename = "new file";
	icu::UnicodeString buffer;

	if (args.size() > 1)
	{
		filename = args[1];
	}
	
	std::string title = std::format(" Text Editor ({}) ", filename);
	std::string left_msg = std::format("{} - X lines (modified)", filename);
	std::string right_msg = "3/4";

	std::stringstream ss;
	ss << CSI_CODE(33);
	ss << TermUtils::line(1, 1, 1, height - 1, "~");
	ss << CSI_RESET;
	ss << TermUtils::status_line(height, width, left_msg, right_msg);
	ss << MOVE_CURSOR(1, 1);
	proc.put("{}", ss.str());

	while (true)
	{
		std::optional<com::CommandQuery> opt_input = proc.read<com::CommandQuery>();

		if (!opt_input.has_value())
		{
			co_await os.wait(0.16f);
			continue;
		}

		const com::CommandQuery& input = *opt_input;

		/* First, update terminal parameters if we're being passed configuration data. */
		if (input.has_screen_data())
		{
			com::ScreenData screen = input.screen_data();
			width = screen.size_x();
			height = screen.size_y();
		}

		/* Next, read the actual command and consider it based on first-byte. */
		std::string str_in = input.command();

		if (str_in.length() == 0)
			continue;

		if (str_in[0] == '\x1b' && str_in[1] == '\0')
			break;

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
			if (last_char_start != icu::BreakIterator::DONE)
			{
				buffer.remove(last_char_start);
				proc.put(CSI "D" CSI "X");
			}

			continue;
		}

		if (str_in.back() == '\0')
			str_in.pop_back();

		icu::UnicodeString chunk = icu::UnicodeString::fromUTF8(str_in);
		buffer.append(chunk);
		
		std::string writeback;
		icu::UnicodeString ret_str = chunk.unescape();
		ret_str.toUTF8String(writeback);
		proc.put("{}", writeback);
	}

	/* Trim any whitespace from the buffer string, convert the result back to utf8,
		and clear the buffer before sending it for processing. */
	std::string out_cmd;
	buffer.trim();
	buffer.toUTF8String(out_cmd);
	buffer.remove();

	com::CommandReply end_msg;
	end_msg.set_con_mode(com::ConsoleMode::Cooked);
	end_msg.set_configure(true);
	end_msg.set_reply(END_ALT_SCREEN_BUFFER "Goodbye...\n");

	proc.write(end_msg);

	co_return 0;
}