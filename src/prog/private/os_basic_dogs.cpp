#include "os_basic.h"

#include "filesystem.h"
#include "users_mgr.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>

auto chunk_by_size(std::string_view sv, std::size_t chunk_size) 
{
    auto count = (sv.size() + chunk_size - 1) / chunk_size;

    return std::views::iota(std::size_t{0}, count)
	| std::views::transform([=](std::size_t i)
	{
		return sv.substr(i * chunk_size, chunk_size); 
	});
}

ProcessTask Programs::CmdDogs(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = proc.owning_os->get_filesystem();
	FilePath pwd(proc.get_var("PWD"));

	if (fs == nullptr)
	{
		proc.errln("No file system.");
		co_return 2;
	}

	CLI::App app{"Watch VT100 animations straight in your terminal!"};
	app.allow_windows_style_options(false);

	struct DogsArgs
	{
		FilePath path{};
		float delay{0};
		float framerate{20.f};
		float hold{2.f};
		std::size_t chunk{0};
	} params{};


	app.add_option("-p,--path,path", params.path, "The file to run (full or relative paths)")->required();

	auto framerate = app.add_option("-r", params.framerate, "The framerate of the app")->capture_default_str();
	app.add_option("-d", params.delay, "The time to wait per frame")->excludes(framerate);

	app.add_option("-w", params.hold, "The time to hold on the last frame")->capture_default_str();
	app.add_option("-s", params.chunk, "Chunk the string based on a fixed line length instead");

	//app.add_flag("-r,--recurse", params.recurse, "Delete files recursively");

	try
	{
		std::ranges::reverse(args);
		args.pop_back();
        app.parse(std::move(args));
    }
	catch(const CLI::ParseError& e)
	{
		int res = app.exit(e, proc.s_out, proc.s_err);
        co_return res;
    }

	int32_t width = proc.get_var<int32_t>("TERM_W");
	int32_t height = proc.get_var<int32_t>("TERM_H");

	if (width == 0 || height == 0)
	{
		proc.warnln("The terminal width/height parameter needs to be set for dogs.");
		co_return 1;
	}

	if (params.path.is_relative())
		params.path.make_absolute(pwd);

	com::CommandReply begin_msg;
	begin_msg.set_reply(
		BEGIN_ALT_SCREEN_BUFFER 
		CLEAR_CURSOR 
		CLEAR_SCREEN
		HIDE_CURSOR);

	com::CommandReply end_msg;
	end_msg.set_reply(
		END_ALT_SCREEN_BUFFER
		SHOW_CURSOR 
		"Dogs finished...\n");

	auto exp_fd = proc.fs.open(params.path, FileAccessFlags::Read);
	if (not exp_fd)
	{
		proc.errln("Failed to open file: {}.", exp_fd.error().message());
		co_return 1;
	}

	auto exp_read = proc.fs.read(*exp_fd);
	if (not exp_read)
	{
		proc.errln("Failed to read from file: {}.", exp_read.error().message());
		co_return 1;
	}

	if (params.framerate == 0 && params.delay == 0)
	{
		proc.errln("You need to set a framerate above 0!");
		co_return 1;
	}

	/* Valid read, begin program here: */
	proc.write(begin_msg);

	float actual_delay = std::invoke([&]()
	{
		if (params.delay > 0)
			return params.delay;

		return 1.f / params.framerate;
	});

	std::size_t frames = 0;
	if (params.chunk > 0)
	{
		for (auto&& chunk : chunk_by_size(*exp_read, params.chunk))
		{
			co_await os.wait(actual_delay);
        	proc.put("{}", std::string_view(chunk));
			++frames;
    	}
	}
	else
	{
		for (auto&& line : std::views::split(*exp_read, '\n'))
		{
			co_await os.wait(actual_delay);
			proc.put("{}", std::string_view(line));
			++frames;
		}
	}

	co_await proc.wait(params.hold);
	proc.write(end_msg);

	if (frames <= 1)
	{
		proc.warnln("Splitting on newline gave only a single frame. Try running with -s <NUM> to split manually.");
	}

	co_return 0;
}