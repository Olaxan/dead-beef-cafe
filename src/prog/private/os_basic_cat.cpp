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

	int32_t success_count{0};

	for (auto& path : params.paths)
	{
		if (path.is_relative())
			path.prepend(proc.get_var("PWD"));

		if (params.paths.size() > 1)
			proc.putln("{}:", path);

		if (auto exp_fd = proc.fs.open(path, FileAccessFlags::Read))
		{
			if (auto exp_read = proc.fs.read(*exp_fd))
			{
				success_count++;
				proc.putln("{}", *exp_read);
			}
			else
			{
				proc.warnln("Failed to read from '{}': {}.", path, exp_read.error().message());	
			}
		}
		else
		{
			proc.warnln("Failed to open '{}': {}.", path, exp_fd.error().message());
		}
	}

	co_return success_count == 0;
}