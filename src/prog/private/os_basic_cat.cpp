#include "os_basic.h"

#include "filesystem.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>

ProcessTask Programs::CmdCat(Proc& proc, std::vector<std::string> args)
{
	FileSystem* fs = proc.owning_os->get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No file system.");
		co_return 2;
	}

	CLI::App app{"A utility that is designed for string concatenation, but mainly used to print file contents."};
	app.allow_windows_style_options(false);

	struct CatArgs
	{
		std::vector<FilePath> paths{};
	} params{};

	app.add_option("-p,--path,path", params.paths, "Files to display (full or relative paths)")->required();

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

	for (auto& path : params.paths)
	{
		if (path.is_relative())
			path.prepend(proc.get_var("SHELL_PATH"));

		if (params.paths.size() > 1)
			proc.putln("{}:", path);

		if (auto [fid, ptr, err] = fs->open(path, FileAccessFlags::Read); err == FileSystemError::Success)
		{
			proc.putln("{}", ptr->get_view());
		}
		else
		{
			proc.warnln("Failed to open '{}' ({}): {}.", path, fid, FileSystem::get_fserror_name(err));
		}
	}

	co_return 0;
}