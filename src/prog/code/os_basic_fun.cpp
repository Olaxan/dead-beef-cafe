#include "os_basic.h"
#include "term_utils.h"

ProcessTask Programs::CmdSnake(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	com::CommandReply begin_msg;
	begin_msg.set_con_mode(com::ConsoleMode::Raw);
	begin_msg.set_configure(true);
	begin_msg.set_reply(
		BEGIN_ALT_SCREEN_BUFFER 
		CLEAR_CURSOR 
		CLEAR_SCREEN 
		"SNAKE MODE ('stop' to exit): ");

	proc.write(begin_msg);

	while (true)
	{
		if (std::optional<com::CommandQuery> opt_query = proc.read<com::CommandQuery>(); opt_query.has_value())
		{
			const std::string& query = opt_query->command();
			
			if (query.size() == 0)
				continue;

			proc.put("{}", query);

			if (query[0] == 'q')
				break;

		}

		co_await os.wait(0.1f);
	}

	com::CommandReply end_msg;
	end_msg.set_reply(END_ALT_SCREEN_BUFFER "Exiting...\n");
	end_msg.set_con_mode(com::ConsoleMode::Cooked);
	end_msg.set_configure(true);
	proc.write(end_msg);

	co_return 0;
}