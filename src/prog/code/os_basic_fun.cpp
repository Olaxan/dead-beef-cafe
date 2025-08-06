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
	
	proc.put("{}", TermUtils::frame(1, 1, width , height, " Snake Mode 0.25 ('q' to exit) "));

	int32_t x_ = 5;
	int32_t y_ = 5;
	int32_t x_speed_ = 1;
	int32_t y_speed_ = 0;

	while (true)
	{
		co_await os.wait(0.16f);

		if (std::optional<com::CommandQuery> opt_query = proc.read<com::CommandQuery>(); opt_query.has_value())
		{
			const std::string& query = opt_query->command();
			
			if (query.size() == 0)
				continue;

			if (query[0] == 'q')
				break;

			bool up_pressed = (query.compare(0, 3, CURSOR_UP) == 0);
			bool down_pressed = (query.compare(0, 3, CURSOR_DOWN) == 0);
			bool left_pressed = (query.compare(0, 3, CURSOR_LEFT) == 0);
			bool right_pressed = (query.compare(0, 3, CURSOR_RIGHT) == 0);

			if (up_pressed)
			{
				y_speed_ = -1;
				x_speed_ = 0;
			}
			if (down_pressed)
			{
				y_speed_ = 1;
				x_speed_ = 0;
			}
			if (left_pressed)
			{
				x_speed_ = -1;
				y_speed_ = 0;
			}
			if (right_pressed)
			{
				x_speed_ = 1;
				y_speed_ = 0;
			}
		}

		//proc.put(CSI "X");
		proc.put(MOVE_CURSOR_PLACEHOLDER "😈", y_, x_);
		x_ = (x_ + x_speed_) % width;
		y_ = (y_ + y_speed_) % height;
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

ProcessTask Programs::CmdDogs(Proc& proc, std::vector<std::string> args)
{
	std::vector<std::string> dog_lines = 
	{
		ESC "[2J",
		ESC "[1;25H          __ ",
		ESC "[2;25H        _/ o\\__,",
		ESC "[3;25H       /   ____/",
		ESC "[4;25H      /   /",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H \\_/   /---\\\\-/ o\\__",
		ESC "[7;25H    \\ |=  ___  ____/  ",
		ESC "[8;25H    | /| /   ||",
		ESC "[9;25H    ||_||    ||_",
		ESC "[10;25H ~~~~~~~~~~~~~~~~~~~~~~~~~",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                          ",
		ESC "[2;25H        _/ -\\__",
		ESC "[5;25H     /   /\\\\   __",
		ESC "[6;25H  \\_/   /--\\\\-/ O\\__ ",
		ESC "[7;25H    |  /  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H  _/   /---\\\\-/ o\\__",
		ESC "[7;25H /  \\ |=  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H     /   /\\\\   __",
		ESC "[6;25H  \\_/   /--\\\\-/ O\\__ ",
		ESC "[7;25H    |  /  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H  _/   /---\\\\-/ o\\__",
		ESC "[7;25H /  \\ |=  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H     /   /\\\\   __",
		ESC "[6;25H  \\_/   /--\\\\-/ O\\__ ",
		ESC "[7;25H    |  /  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H  _/   /---\\\\-/ o\\__",
		ESC "[7;25H /  \\ |=  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H     /   /\\\\   __",
		ESC "[6;25H  \\_/   /--\\\\-/ O\\__ ",
		ESC "[7;25H    |  /  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H  _/   /---\\\\-/ o\\__",
		ESC "[7;25H /  \\ |=  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H     /   /\\\\   __",
		ESC "[6;25H  \\_/   /--\\\\-/ O\\__ ",
		ESC "[7;25H    |  /  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H  _/   /---\\\\-/ o\\__",
		ESC "[7;25H /  \\ |=  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H     /   /\\\\   __",
		ESC "[6;25H  \\_/   /--\\\\-/ O\\__ ",
		ESC "[7;25H    |  /  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H  _/   /---\\\\-/ o\\__",
		ESC "[7;25H /  \\ |=  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H     /   /\\\\   __",
		ESC "[6;25H  \\_/   /--\\\\-/ O\\__ ",
		ESC "[7;25H    |  /  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H  _/   /---\\\\-/ o\\__",
		ESC "[7;25H /  \\ |=  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H     /   /\\\\   __",
		ESC "[6;25H  \\_/   /--\\\\-/ O\\__ ",
		ESC "[7;25H    |  /  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H  _/   /---\\\\-/ o\\__",
		ESC "[7;25H /  \\ |=  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H     /   /\\\\   __",
		ESC "[6;25H  \\_/   /--\\\\-/ O\\__ ",
		ESC "[7;25H    |  /  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H  _/   /---\\\\-/ o\\__",
		ESC "[7;25H /  \\ |=  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H     /   /\\\\   __",
		ESC "[6;25H  \\_/   /--\\\\-/ O\\__ ",
		ESC "[7;25H    |  /  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[5;25H    _-   /\\\\   __",
		ESC "[6;25H  _/   /---\\\\-/ o\\__",
		ESC "[7;25H /  \\ |=  ___  ____/  ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                          ",
		ESC "[20;25H                                                        ",
		ESC "[20;25H                   ",
		ESC "[2;25H        _/ .\\__,  Arrrgh",
		ESC "[3;25H       /   ____/    _/",
		ESC "[4;25H      /   /",
		ESC "[5;25H     /   /\\\\   __   ",
		ESC "[6;25H  \\_/   /--\\\\-/ *\\__  Woof.",
		ESC "[7;25H    |  /  ___  ____/  _/ ",
		ESC "[21;25H",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[6;25H  \\",
		ESC "[7;25H   ",
		ESC "[20;25H                                                        ",
		ESC "[6;25H   ",
		ESC "[7;25H  /",
		ESC "[21;25H"
	};

	OS& os = *proc.owning_os;

	int32_t width = proc.get_var<int32_t>("TERM_W");
	int32_t height = proc.get_var<int32_t>("TERM_H");

	if (width == 0 || height == 0)
	{
		proc.warnln("The terminal width/height parameter needs to be set for dogs.");
		co_return 1;
	}

	com::CommandReply begin_msg;
	begin_msg.set_con_mode(com::ConsoleMode::Raw);
	begin_msg.set_configure(true);
	begin_msg.set_reply(
		BEGIN_ALT_SCREEN_BUFFER 
		CLEAR_CURSOR 
		CLEAR_SCREEN
		HIDE_CURSOR);

	proc.write(begin_msg);
	
	for (const std::string& line : dog_lines)
	{
		co_await os.wait(0.1f);
		proc.put("{}", line);
	}

	com::CommandReply end_msg;
	end_msg.set_con_mode(com::ConsoleMode::Cooked);
	end_msg.set_configure(true);
	end_msg.set_reply(
		END_ALT_SCREEN_BUFFER
		SHOW_CURSOR 
		"Dogs finished...\n");

	proc.write(end_msg);

	co_return 0;
}
