#include "os_basic.h"
#include "term_utils.h"

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/usearch.h>
#include <unicode/ustring.h>
#include <unicode/ustream.h>
#include <unicode/brkiter.h>

#include <algorithm>

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

	struct SnakeCoord
	{
		int32_t x = 0;
		int32_t y = 0;

		SnakeCoord operator + (const SnakeCoord& other) const
		{
			return { x + other.x, y + other.y };
		}

		auto operator <=> (const SnakeCoord& other) const = default;
	};

	int32_t frame = 0;
	SnakeCoord speed{1, 0};
	std::size_t snake_len = 1;
	std::deque<SnakeCoord> snake_body{ { width / 2, height / 2 } };

	SnakeCoord apple = snake_body.front() + SnakeCoord{10, 0};

	const std::string snake_str = "SNAKE";
	const std::string snake_str2 = "SNAKES-ARE-ELONGATED-LIMBLESS-REPTILES-OF-THE-SUBORDER-SERPENTES-CLADISTICALLY-SQUAMATES-SNAKES-ARE-ECTOTHERMIC-AMNIOTE-VERTEBRATES-COVERED-IN-OVERLAPPING-SCALES-MUCH-LIKE-OTHER-MEMBERS-OF-THE-GROUP-MANY-SPECIES-OF-SNAKES-HAVE-SKULLS-WITH-SEVERAL-MORE-JOINTS-THAN-THEIR-LIZARD-ANCESTORS-AND-RELATIVES-ENABLING-THEM-TO-SWALLOW-PREY-MUCH-LARGER-THAN-THEIR-HEADS-CRANIAL-KINESIS-TO-ACCOMMODATE-THEIR-NARROW-BODIES-SNAKES-PAIRED-ORGANS-SUCH-AS-KIDNEYS-APPEAR-ONE-IN-FRONT-OF-THE-OTHER-INSTEAD-OF-SIDE-BY-SIDE-AND-MOST-ONLY-HAVE-ONE-FUNCTIONAL-LUNG-SOME-SPECIES-RETAIN-A-PELVIC-GIRDLE-WITH-A-PAIR-OF-VESTIGIAL-CLAWS-ON-EITHER-SIDE-OF-THE-CLOACA-LIZARDS-HAVE-INDEPENDENTLY-EVOLVED-ELONGATE-BODIES-WITHOUT-LIMBS-OR-WITH-GREATLY-REDUCED-LIMBS-AT-LEAST-TWENTY-FIVE-TIMES-VIA-CONVERGENT-EVOLUTION-LEADING-TO-MANY-LINEAGES-OF-LEGLESS-LIZARDS-THESE-RESEMBLE-SNAKES-BUT-SEVERAL-COMMON-GROUPS-OF-LEGLESS-LIZARDS-HAVE-EYELIDS-AND-EXTERNAL-EARS-WHICH-SNAKES-LACK-ALTHOUGH-THIS-RULE-IS-NOT-UNIVERSAL-SEE-AMPHISBAENIA-DIBAMIDAE-AND-PYGOPODIDAE.";

	while (true)
	{
		++frame;

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
			bool going_horizontal = speed.x != 0;
			bool going_vertical = speed.y != 0;

			if (up_pressed && going_horizontal)
			{
				speed.y = -1;
				speed.x = 0;
			}
			if (down_pressed && going_horizontal)
			{
				speed.y = 1;
				speed.x = 0;
			}
			if (left_pressed && going_vertical)
			{
				speed.x = -1;
				speed.y = 0;
			}
			if (right_pressed && going_vertical)
			{
				speed.x = 1;
				speed.y = 0;
			}
		}

		std::stringstream ss;

		const SnakeCoord& head = snake_body.front();
		SnakeCoord new_head = head + speed;

		/* We collect an apple! */
		if (new_head == apple)
		{
			apple.x = 1 + std::rand() % (width - 2);
			apple.y = 1 + std::rand() % (height - 2);
			++snake_len;
		}

		bool crash_self = (std::find(snake_body.begin(), snake_body.end(), new_head) != snake_body.end());
		bool crash_wall = (new_head.x <= 1 || new_head.x >= width - 1 || new_head.y <= 1 || new_head.y >= height - 1);

		/* Self-crash! */
		if (crash_self || crash_wall)
		{
			proc.put(MOVE_CURSOR(5,5) "You are SNAKE JAM!" MOVE_CURSOR(6, 6) "{} points.", snake_len);
			co_await os.wait(5);
			break;
		}

		snake_body.push_front(new_head);
		while (snake_body.size() > snake_len)
		{
			SnakeCoord& back = snake_body.back();
			ss << TermUtils::pixel(back.x, back.y, " ");
			snake_body.pop_back();
		}

		/* Add the head afterwards so that a stationary snake is still drawn. */
		std::size_t snake_idx = frame % std::min(snake_str.length(), snake_len);
		std::string new_snake = std::format("{}", snake_str[snake_idx]);
		ss << TermUtils::pixel(new_head.x ,new_head.y, new_snake);
		ss << TermUtils::pixel(apple.x, apple.y, "O");

		proc.put("{}", ss.str());

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
