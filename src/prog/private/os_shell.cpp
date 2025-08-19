#include "os_basic.h"
#include "os_fileio.h"
#include "os_shell.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>
#include <format>
#include <ranges>
#include <functional>

EagerTask<int32_t> ShellUtils::Exec(Proc& proc, std::vector<std::string>&& args)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();

	std::string_view name{*std::begin(args)};

	/* If the last argument is &, run this process in the background. */
	bool run_in_background = std::invoke([&]
	{
		if (std::string_view(args.back()) == "&")
		{
			args.pop_back();
			return true;
		}
		return false;
	});

	/* If >> FILENAME is in the arguments, try to open the file as a output redirector. */
	WriterFn redirect_writer = std::invoke([&]
	{
		WriterFn out{nullptr};

		auto begin_find = std::ranges::find(args, ">>");		

		if (begin_find == args.end())
			return out;

		auto end_find = std::ranges::next(begin_find);

		if (end_find != args.end())
		{
			FilePath where{*end_find};

			if (where.is_relative())
				where.make_absolute(proc.get_var("PWD"));

			if (auto [fid, ptr, err] = FileUtils::open(proc, std::move(where), FileAccessFlags::Create | FileAccessFlags::Write); 
			err == FileSystemError::Success)
			{
				out = [file = std::move(ptr)](const std::string& line)
				{
					file->append(line);
				};
			}
		}

		args.erase(begin_find, std::ranges::next(end_find));

		return out;
	});

	/* Try to find a file that matches on the PATH (or directly specified) */
	auto match = [&] -> std::optional<FilePath>
	{
		std::vector<std::string> candidates = proc.get_var("PATH")
		| std::views::split(';')
		| std::views::transform([&name](auto&& path) { return std::format("{}/{}", std::string_view(path), name); })
		| std::ranges::to<std::vector>();

		candidates.emplace_back(name);

		for (auto&& path : candidates)
		{
			if (auto [fid, err] = FileUtils::query(proc, path, FileAccessFlags::Execute); fid != 0)
			{
				return fs->get_path(fid);
			}
		}

		return std::nullopt;
	}();
	
	if (!match)
	{
		proc.warnln("'{}': No such file or directory.", name);
		co_return 1;
	}

	if (auto [fid, ptr, err] = FileUtils::open(proc, *match, FileAccessFlags::Execute); err == FileSystemError::Success)
	{
		FileMeta* meta = fs->get_metadata(fid);
		assert(meta);

		bool setuid = fs->has_flag<ExtraFileFlags>(meta->extra, ExtraFileFlags::SetUid);
		bool setgid = fs->has_flag<ExtraFileFlags>(meta->extra, ExtraFileFlags::SetGid);
		int32_t exec_uid = setuid ? meta->owner_uid : proc.get_uid();
		int32_t exec_gid = setgid ? meta->owner_gid : proc.get_gid();

		OS::CreateProcessParams params
		{
			.writer = std::move(redirect_writer),
			.leader_id = proc.get_pid(),
			.uid = exec_uid,
			.gid = exec_gid
		};

		auto&& prog = ptr->get_executable();
		if (!prog)
		{
			proc.errln("exec: no program entry point detected.");
			co_return 1;
		}

		if (run_in_background)
		{
			os.run_process(prog, std::move(args), std::move(params));
			co_return 0;
		}
		else
		{
			co_return (co_await os.run_process(prog, std::move(args), std::move(params)));
		}
	}
	else
	{
		proc.warnln("exec: failed to open '{}': {}.", *match, FileSystem::get_fserror_name(err));
		co_return 1;
	}
	
	co_return 1;
}