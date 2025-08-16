#include "os_basic.h"

#include "filesystem.h"
#include "os_fileio.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>


ProcessTask Programs::CmdRemoveFile(Proc& proc, std::vector<std::string> args)
{

	FileSystem* fs = proc.owning_os->get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No file system.");
		co_return 2;
	}

	CLI::App app{"Utility to remove files and directories."};
	app.allow_windows_style_options(false);

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

	int32_t files_removed = 0;

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
					if (params.verbose) { proc.warnln("Deleting root directory '/' (you asked for it!)"); };
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

	for (auto& path : params.paths)
	{
		if (path.is_relative())
			path.prepend(proc.get_var("PWD"));

		FileUtils::remove(proc, path, remover);
	}

	proc.putln("Removed {0} file(s).", files_removed);
	co_return static_cast<int32_t>(files_removed == 0);
}