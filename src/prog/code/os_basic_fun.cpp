#include "os_basic.h"
#include "term_utils.h"

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/usearch.h>
#include <unicode/ustring.h>
#include <unicode/ustream.h>
#include <unicode/brkiter.h>

ProcessTask Programs::CmdSnake(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	int32_t width = proc.get_var<int32_t>("TERM_W");
	int32_t height = proc.get_var<int32_t>("TERM_H");

	if (width == 0 || height == 0)
	{
		proc.warnln("The terminal width/height parameter needs to be set for SNAKE MODE.");
		co_return 1;
	}

	com::CommandReply begin_msg;
	begin_msg.set_con_mode(com::ConsoleMode::Raw);
	begin_msg.set_configure(true);
	begin_msg.set_reply(
		BEGIN_ALT_SCREEN_BUFFER 
		CLEAR_CURSOR 
		CLEAR_SCREEN
		HIDE_CURSOR
		MOVE_CURSOR(5, 5));

	proc.write(begin_msg);
	
	proc.put("{}", TermUtils::frame(width, height));

	int32_t curr = 5;

	while (true)
	{
		co_await os.wait(0.25f);

		if (std::optional<com::CommandQuery> opt_query = proc.read<com::CommandQuery>(); opt_query.has_value())
		{
			const std::string& query = opt_query->command();
			
			if (query.size() == 0)
				continue;

			if (query[0] == 'q')
				break;

		}

		proc.put(CSI "X");
		curr = ++curr % width;
		proc.put(MOVE_CURSOR_PLACEHOLDER "😈", curr, 5);
	}
	
	com::CommandReply end_msg;
	end_msg.set_con_mode(com::ConsoleMode::Cooked);
	end_msg.set_configure(true);
	end_msg.set_reply(
		END_ALT_SCREEN_BUFFER
		SHOW_CURSOR 
		"Exiting...\n");

	proc.write(end_msg);

	co_return 0;
}