#include "os_basic.h"

#include "device.h"
#include "disk.h"
#include "filesystem.h"
#include "navigator.h"
#include "os_fileio.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>


ProcessTask Programs::CmdChangeDir(Proc& proc, std::vector<std::string> args)
{
	if (args.size() != 2)
	{
		proc.warnln("Usage: cd [file/directory]");
		co_return 1;
	}

	if (Navigator* nav = proc.get_data<Navigator>())
	{
		co_return nav->go_to(args[1]) ? 0 : 1;
	}

	proc.errln("No file navigator.");
	co_return 1;
}

ProcessTask Programs::CmdMakeDir(Proc& proc, std::vector<std::string> args)
{
	if (args.size() != 2)
	{
		proc.warnln("Usage: mkdir [name]");
		co_return 1;
	}

	if (Navigator* nav = proc.get_data<Navigator>())
	{
		auto [fid, ptr, err] = nav->create_directory(args[1]);
		if (err == FileSystemError::Success)
		{
			proc.putln("Created directory '{}':{} under {}.", args[1], fid, nav->get_current());
			co_return 0;
		}
		else
		{
			proc.warnln("Failed to create directory: {}.", FileSystem::get_fserror_name(err));
			co_return static_cast<int32_t>(err);
		}
	}

	proc.errln("No file navigator.");
	co_return 1;
}

ProcessTask Programs::CmdMakeFile(Proc& proc, std::vector<std::string> args)
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


	app.add_option("-p,--path,path", params.paths, "Files to remove (full or relative paths)")->required();

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
	
	FileAccessFlags flags = FileAccessFlags::Write;
	if (!params.no_create) { flags |= FileAccessFlags::Create; }

	for (auto& path : params.paths)
	{
		if (path.is_relative())
			path.prepend(proc.get_var("SHELL_PATH"));

		if (auto [fid, ptr, err] = FileUtils::open(proc, path, flags); err == FileSystemError::Success)
		{
			fs->file_set_modified_now(fid);
			++num_updated;
		}
	}

	proc.putln("Updated {} file(s).", num_updated);
	co_return 0;
}

ProcessTask Programs::CmdOpenFile(Proc& proc, std::vector<std::string> args)
{
	if (Navigator* nav = proc.get_data<Navigator>())
	{
		proc.putln("Files in '{}': {}", nav->get_dir(), nav->get_files());
		co_return 0;
	}

	proc.errln("No file navigator.");
	co_return 1;
}

ProcessTask Programs::CmdGoUp(Proc& proc, std::vector<std::string> args)
{
	if (Navigator* nav = proc.get_data<Navigator>())
	{
		co_return nav->go_up() ? 0 : 1;
	}

	proc.errln("No file navigator.");
	co_return 1;
}
