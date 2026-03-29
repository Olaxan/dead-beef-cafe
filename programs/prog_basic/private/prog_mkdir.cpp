#include "os_basic.h"

#include "device.h"
#include "disk.h"
#include "filesystem.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>

#include <iso646.h>

ProcessTask Programs::CmdMakeDir(Proc& proc, std::vector<std::string> args)
{
	FileSystem* fs = proc.owning_os->get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No file system.");
		co_return 2;
	}

	CLI::App app{"Create directories."};
	app.allow_windows_style_options(false);

	struct MkDirArgs
	{
		bool no_create{false};
		std::string date_instead{};
		std::vector<FilePath> paths{};
	} params{};


	app.add_option("-p,--path,path", params.paths, "Directories to create (full or relative paths)")->required();

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
	
	FileAccessFlags flags = FileAccessFlags::Write | FileAccessFlags::Create;

	for (auto& path : params.paths)
	{
		if (path.is_relative())
			path.prepend(proc.get_var("PWD"));

		if (auto exp_fd = proc.fs.open(path, flags))
		{
			NodeIdx h = proc.fs.get_node(*exp_fd);
			fs->file_set_directory_flag(h, true);
			fs->file_set_permissions(h, 7, 5, 5);
			proc.fs.close(*exp_fd);
			++num_updated;
		}
	}

	proc.putln("Created {} directories.", num_updated);
	co_return (num_updated == 0);
}