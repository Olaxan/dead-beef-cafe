#include "os_basic.h"

#include "device.h"
#include "disk.h"
#include "filesystem.h"
#include "navigator.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>

ProcessTask Programs::CmdList(Proc& proc, std::vector<std::string> args)
{
	if (Navigator* nav = proc.get_data<Navigator>())
	{
		proc.putln("Files in '{}': {}", nav->get_dir(), nav->get_files());
		co_return 0;
	}

	proc.errln("No file navigator.");
	co_return 1;
}

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
		auto [fid, err] = nav->create_directory(args[1]);
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
	if (args.size() != 2)
	{
		proc.warnln("Usage: touch [filename]");
		co_return 1;
	}

	if (Navigator* nav = proc.get_data<Navigator>())
	{
		auto [fid, err] = nav->create_file(args[1]);
		if (err == FileSystemError::Success)
		{
			proc.putln("Created file '{}':{} under {}.", args[1], fid, nav->get_current());
			co_return 0;
		}
		else
		{
			proc.warnln("Failed to create file: {}.", FileSystem::get_fserror_name(err));
			co_return static_cast<int32_t>(err);
		}
	}

	proc.errln("No file navigator.");
	co_return 1;
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

ProcessTask Programs::CmdRemoveFile(Proc& proc, std::vector<std::string> args)
{

	FileSystem* fs = proc.owning_os->get_filesystem();

	CLI::App app{"Utility to remove files and directories."};

	struct RmArgs
	{
		bool recurse = false;
		bool force = false;
		bool verbose = false;
		bool no_preserve_root = false;
		std::vector<FilePath> paths{};
	} params{};


	app.add_option("-p,--path,path", params.paths, "Files to remove (full or relative paths)")->required();

	app.add_flag("-r,--recurse", params.recurse, "Delete files recursively");
	app.add_flag("-f,--force", params.force, "Don't exit on error");
	app.add_flag("-v,--verbose", params.verbose, "Tell us exactly what is going on");
	app.add_flag("--no-preserve-root", params.no_preserve_root, "Do not treat the root directory '/' as protected from deletion");

	std::ranges::reverse(args);
	args.pop_back();

	int32_t files_removed = 0;

	auto remover = [&proc, &files_removed, &params](const FileSystem& fs, const FilePath& path, FileSystemError code) -> bool
	{
		switch (code)
		{
			case (FileSystemError::Success):
			{
				++files_removed;
				if (params.verbose) { proc.putln("Removed '{}'.", path); }
				return true;
			}
			case (FileSystemError::FolderNotEmpty):
			{
				if (params.recurse)
				{
					if (params.verbose) { proc.putln("Deleting files in '{}':", path); };
					return true;
				}
				else break;
			}
			case (FileSystemError::PreserveRoot):
			{
				if (params.no_preserve_root)
				{
					if (params.verbose) { proc.warnln("Deleting root directory '/' -- you asked for it!"); };
					return true;
				}
				else break;
			}
			default:
			{
				if (params.verbose) { proc.warnln("Failed to remove file '{}': {}.", path, FileSystem::get_fserror_name(code)); }
				return params.force;
			}
		}

		proc.warnln("Failed to remove file '{}': {}.", path, FileSystem::get_fserror_name(code));
		return false;
	};

	try
	{
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

		fs->remove_file(path, remover);
	}

	proc.putln("Removed {0} file(s).", files_removed);
	co_return static_cast<int32_t>(files_removed == 0);
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
