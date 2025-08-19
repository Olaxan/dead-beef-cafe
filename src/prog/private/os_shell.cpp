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
				std::println("Redirecting to '{}'", *end_find);
				out = [file = std::move(ptr)](const std::string& line)
				{
					file->append(line);
				};
			}
		}

		args.erase(begin_find, std::ranges::next(end_find));

		return out;
	});
	

	for (auto str : proc.get_var("PATH") | std::views::split(';'))
	{
		FilePath prog_path(std::format("{}/{}", std::string_view(str), name));

		if (auto [fid, ptr, err] = FileUtils::open(proc, prog_path, FileAccessFlags::Execute); err == FileSystemError::Success)
		{
			/* Get setuid, setgid bits from file system. */
			auto [exec_uid, exec_gid] = std::invoke([&fs, &fid, &proc]
			{
				if (FileMeta* meta = fs->get_metadata(fid))
				{
					bool setuid = fs->has_flag<ExtraFileFlags>(meta->extra, ExtraFileFlags::SetUid);
					bool setgid = fs->has_flag<ExtraFileFlags>(meta->extra, ExtraFileFlags::SetGid);
					return std::make_pair(
						setuid ? meta->owner_uid : proc.get_uid(),
						setgid ? meta->owner_gid : proc.get_gid()
					);
				}
				else 
				{
					return std::make_pair(proc.get_uid(), proc.get_gid());
				}
			});

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
				proc.errln("No program entry point detected!");
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
	}

	proc.warnln("'{}': No such file or directory.", name);
	co_return 1;
}