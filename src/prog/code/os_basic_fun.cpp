#include "os_basic.h"


ProcessTask Programs::CmdSnake(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;

	com::CommandReply reply;
	reply.set_reply("SNAKE MODE ('stop' to exit): ");
	proc.write(reply);

	while (true)
	{
		if (std::optional<com::CommandQuery> opt_query = proc.read<com::CommandQuery>(); opt_query.has_value())
		{
			const std::string& query = opt_query->command();
			
			if (strcmp(query.c_str(), "stop") == 0)
				co_return 0;

		}

		co_await os.wait(0.1f);
	}

	co_return 0;
}