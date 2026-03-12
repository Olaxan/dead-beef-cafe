#include "os_fileio.h"
#include "os_shell.h"

#include "CLI/CLI.hpp"

#include <string>
#include <vector>
#include <print>
#include <format>
#include <ranges>
#include <algorithm>
#include <functional>
#include <optional>

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
	auto redirect_to = std::invoke([&]() -> std::optional<FilePath>
	{
		WriterFn out{nullptr};

		auto begin_find = std::ranges::find(args, ">>");		

		if (begin_find == args.end())
			return std::nullopt;

		auto end_find = std::ranges::next(begin_find);

		if (end_find == args.end())
			return std::nullopt;

		FilePath where{*end_find};

		if (where.is_relative())
			where.make_absolute(proc.get_var("PWD"));

		args.erase(begin_find, std::ranges::next(end_find));

		return where;
	});

	InvokeFn invoker = [redirect_to](Proc* new_proc)
	{
		if (redirect_to)
		{
			FileAccessFlags flags = FileAccessFlags::Create | FileAccessFlags::Write | FileAccessFlags::Append;

			if (auto exp_fd = new_proc->fs.open(*redirect_to, flags))
			{
				new_proc->set_writer([new_proc, fd = exp_fd.value()](const std::string& line)
				{
					new_proc->fs.write(fd, line);
				});
			}
		}
	};

	/* Try to find a file that matches on the PATH (or directly specified) */
	auto match = std::invoke([&]() -> std::optional<FilePath>
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
	});
	
	if (!match)
	{
		proc.warnln("'{}': No such file or directory.", name);
		co_return 1;
	}

	if (auto exp_fd = proc.fs.open(*match, FileAccessFlags::Execute))
	{
		FileDescriptor fd = exp_fd.value();

		auto exp_meta = proc.fs.get_metadata(fd);
		if (!exp_meta)
		{
			proc.errln("exec: failed to retrieve program metadata: {}.", exp_meta.error().message());
			co_return 1;
		}

		FileMeta* meta = exp_meta.value();
		bool setuid = fs->has_flag<ExtraFileFlags>(meta->extra, ExtraFileFlags::SetUid);
		bool setgid = fs->has_flag<ExtraFileFlags>(meta->extra, ExtraFileFlags::SetGid);
		int32_t exec_uid = setuid ? meta->owner_uid : proc.get_uid();
		int32_t exec_gid = setgid ? meta->owner_gid : proc.get_gid();
	
		OS::CreateProcessParams params
		{
			.invoke = std::move(invoker),
			.leader_id = proc.get_pid(),
			.uid = exec_uid,
			.gid = exec_gid
		};

		auto&& prog = proc.fs.read_exe(fd);
		if (!prog)
		{
			proc.errln("exec: no program entry point detected.");
			co_return 1;
		}

		proc.fs.close(fd);

		if (run_in_background)
		{
			os.run_process(*prog, std::move(args), std::move(params));
			co_return 0;
		}
		else
		{
			co_return (co_await os.run_process(*prog, std::move(args), std::move(params)));
		}
	}
	else
	{
		proc.warnln("exec: failed to open '{}': {}.", *match, exp_fd.error().message());
		co_return 1;
	}
	
	co_return 1;
}