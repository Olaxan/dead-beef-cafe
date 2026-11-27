#include "prog_world.h"

#include "proc.h"
#include "world.h"
#include "host_utils.h"
#include "os.h"
#include "os_basic.h"
#include "file_archive.h"
#include "filesystem_types.h"

#include "CLI/CLI.hpp"

#include <optional>
#include <expected>

#include <iso646.h>

ProcessTask Programs::CmdArchive(Proc& proc, std::vector<std::string> args)
{
	FileArchive* ar = proc.owning_os->get_context().archive;
	ProcFsApi& fs = proc.fs;

	if (not ar)
	{
		proc.errln("Failed to acquire archive.");
		co_return 1;
	}

	std::vector<std::string> tags = ar->get_all_program_tags();
	std::vector<ArHandle> progs = ar->get_all_programs();

	proc.putln("Available programs: {}.", tags);
	std::println("Available programs: {}.", tags);

	for (auto&& tag : tags)
	{
		auto data = ar->get_by_tag(tag)
		.and_then([&](ArHandle h)
		{	
			return ar->try_get_bytes(h);
		});

		if (data)
		{
			FilePath path{std::format("/bin/{}", tag)};
			std::ignore = fs.open(path, FileAccessFlags::ReadWriteExec)
			.and_then([&](FileDescriptor fd) -> DescriptorResult
			{
				std::ignore = fs.write(fd, std::string(*data));
				return fd;
			})
			.or_else([&](std::error_condition err) -> DescriptorResult
			{
				proc.errln("arget: {}.", err.message());
				return std::unexpected(err);
			});
		}
	}

	co_return 1;
}