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

	FileSystem* fs = proc.owning_os->get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No file system.");
		co_return 2;
	}

	FilePath wd = proc.get_var("SHELL_PATH");

	CLI::App app{"Utility to show contents and data about files and directories."};
	app.allow_windows_style_options(false);

	struct RmArgs
	{
		bool show_all = false;
		bool ignore_backups = false;
		bool recurse = false;
		bool list = false;
		std::vector<FilePath> paths{};
	} params{};


	app.add_option("-p,--path,path", params.paths, "Files to list (full or relative paths)");

	app.add_flag("-a,--all", params.show_all, "Don't hide files starting with '.'");
	app.add_flag("-B,--ignore-backups", params.ignore_backups, "Don't list files ending with ~");
	app.add_flag("-R,--recursive", params.recurse, "List subdirectories recursively");
	app.add_flag("-l,--list", params.list, "Shows a more detailed list of files");

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

	if (params.paths.empty())
	{
		params.paths.emplace_back(wd);
	}

	struct LsListData
	{
		uint64_t fid{0};
		uint64_t links{0};
		uint64_t bytes{0};
		std::string flags{};
		std::string owner{};
		std::string mdate{};
		std::string name{};
		bool is_dir{false};
	};

	struct LsListDataWidth
	{
		std::size_t w_fid   = 1;
		std::size_t w_links = 1;
		std::size_t w_bytes = 1;
		std::size_t w_flags = 1;
		std::size_t w_owner = 1;
		std::size_t w_mdate = 1;
	};

	auto expand_format = [](const LsListData& data, LsListDataWidth& format)
	{
		format.w_fid = 	 std::max(format.w_fid,   data.fid   != 0 ? static_cast<std::size_t>(std::log10(data.fid))   + 1 : 0);
		format.w_links = std::max(format.w_links, data.links != 0 ? static_cast<std::size_t>(std::log10(data.links)) + 1 : 0);
		format.w_bytes = std::max(format.w_bytes, data.bytes != 0 ? static_cast<std::size_t>(std::log10(data.bytes)) + 1 : 0);
		format.w_flags = std::max(format.w_flags, data.flags.size());
		format.w_owner = std::max(format.w_owner, data.owner.size());
		format.w_mdate = std::max(format.w_mdate, data.mdate.size());
	};

	auto get_list_data = [&fs](uint64_t fid, const FilePath& path) -> LsListData
	{
		LsListData data{};
		if (fid != 0)
		{
			data.fid = fid;
			data.links = fs->get_links(fid);
			data.bytes = fs->get_bytes(fid);
			data.flags = fs->get_flags(fid);
			data.owner = fs->get_owner(fid);
			data.mdate = fs->get_mdate(fid);
			data.is_dir = fs->is_dir(fid);
			data.name = path.get_name();
		}
		return data;
	};

	auto print_list = [&proc](const std::vector<LsListData>& data, const LsListDataWidth& f)
	{
		std::stringstream ss;
		for (const LsListData& item : data)
		{
			ss << CSI_RESET;
			ss << std::setw(f.w_fid)   << item.fid   << " ";
			ss << std::setw(f.w_flags) << item.flags << " ";
			ss << std::setw(f.w_links) << item.links << " ";
			ss << std::setw(f.w_owner) << item.owner << " ";
			ss << std::setw(f.w_mdate) << item.mdate << " ";
			ss << (item.is_dir ? CSI_CODE(1) CSI_CODE(34) : CSI_RESET);
			ss << item.name << "\n";
		}
		
		proc.put("{}", ss.str());
	};

	auto print_basic = [&proc](const std::vector<LsListData>& data)
	{
		std::stringstream ss;
		for (const LsListData& item : data)
		{
			ss << CSI_RESET;
			ss << (item.is_dir ? CSI_CODE(1) CSI_CODE(34) : CSI_RESET);
			ss << item.name << "\t";
		}
		proc.putln("{}", ss.str());
	};

	for (auto& path : params.paths)
	{
		proc.putln("Files in '{}':", path);

		if (path.is_relative())
			path.prepend(wd);

		LsListDataWidth widths{};
		std::vector<LsListData> data{};
		for (const auto& path : fs->get_paths(path, params.recurse))
		{
			uint64_t fid = fs->get_fid(path);

			if (fid == 0)
				continue;

			if (path.is_backup() && params.ignore_backups)
				continue;

			if (path.is_config() && !params.show_all)
				continue;

			LsListData item = get_list_data(fid, path);
			expand_format(item, widths);
			data.emplace_back(std::move(item));
		}

		if (params.list)
		{
			print_list(data, widths);
		}
		else
		{
			print_basic(data);
		}
	}

	co_return 0;
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
	if (args.size() != 2)
	{
		proc.warnln("Usage: touch [filename]");
		co_return 1;
	}

	if (Navigator* nav = proc.get_data<Navigator>())
	{
		auto [fid, ptr, err] = nav->create_file(args[1]);
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
