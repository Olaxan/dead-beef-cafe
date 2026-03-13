#include "os_basic.h"

#include "device.h"
#include "disk.h"
#include "filesystem.h"
#include "os_fileio.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>


ProcessTask Programs::CmdTouch(Proc& proc, std::vector<std::string> args)
{

	FileSystem* fs = proc.owning_os->get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No file system.");
		co_return 2;
	}

	CLI::App app{"Create files and modify file metadata..."};
	app.allow_windows_style_options(false);

	struct TouchArgs
	{
		bool no_create{false};
		std::string date_instead{};
		std::vector<FilePath> paths{};
	} params{};


	app.add_option("-p,--path,path", params.paths, "Files to update (full or relative paths)")->required();

	app.add_flag("-c,--no-create", params.no_create, "Don't create files that don't exist");
	app.add_option("-d,--date", params.date_instead, "Set the new date directly (a parsable string)");

	int32_t num_updated = 0;
	
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
	
	FileAccessFlags flags = FileAccessFlags::Write | FileAccessFlags::Append;
	if (!params.no_create) { flags |= FileAccessFlags::Create; }

	for (auto& path : params.paths)
	{
		if (path.is_relative())
			path.prepend(proc.get_var("PWD"));

		if (auto exp_fd = proc.fs.open(path, flags))
		{
			std::ignore = proc.fs.write(*exp_fd, "");
			++num_updated;
		}
	}

	proc.putln("Updated {} file(s).", num_updated);
	co_return 0;
}