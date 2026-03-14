#include "os_basic.h"

#include "filesystem.h"
#include "users_mgr.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>

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

	struct RmArgs
	{
		FilePath path{};
		float delay{0.05f};
	} params{};


	app.add_option("-p,--path,path", params.path, "The file to run (full or relative paths)")->required();
	app.add_option("-d", params.delay, "The time to wait per count")->capture_default_str();

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

	/* Valid read, begin program here: */
	proc.write(begin_msg);

	auto dog_lines = std::views::split(*exp_read, '\n');
	
	for (auto&& line : dog_lines)
	{
		co_await os.wait(params.delay);
		proc.put("{}", std::string_view(line));
	}

	proc.write(end_msg);

	co_return 0;
}