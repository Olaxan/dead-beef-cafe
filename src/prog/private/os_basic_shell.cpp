#include "os_basic.h"

#include "device.h"
#include "netw.h"
#include "filesystem.h"
#include "os_input.h"
#include "os_fileio.h"

#include "CLI/CLI.hpp"

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/usearch.h>
#include <unicode/ustring.h>
#include <unicode/ustream.h>
#include <unicode/brkiter.h>

#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <ranges>
#include <functional>


ProcessTask Programs::CmdShell(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No file system!");
		co_return 1;
	}

	auto sock = os.create_socket<CmdSocketServer>();
	if (!os.bind_socket(sock, 22))
	{
		proc.errln("Failed to bind socket 22 for reading.");
		co_return 1;
	}

	/* Register writer functors in the process so that output
	is delivered in the form of command objects via the socket. */
	proc.add_writer<std::string>([sock](const std::string& out_str)
	{
		com::CommandReply reply{};
		reply.set_reply(out_str);
		sock->write(std::move(reply));
	});

	proc.add_writer<com::CommandReply>([sock](const com::CommandReply& out_reply)
	{
		sock->write(out_reply);
	});

	/* Add reader functors so that child processes can listen to our traffic. */
	proc.add_reader<std::string>([sock]() -> std::any
	{
		if (std::optional<com::CommandQuery> opt = sock->read(); opt.has_value())
			return opt->command();
		
		return {};
	});

	/* Async socket reader -> CommandQuery. */
	proc.add_reader<CmdSocketAwaiterServer>([sock]() -> std::any
	{
		return sock->async_read();
	});

	/* Synchronous socket reader -> CommandQuery*/
	proc.add_reader<com::CommandQuery>([sock]() -> std::any
	{
		if (std::optional<com::CommandQuery> opt = sock->read(); opt.has_value())
			return *opt;
		
		return {};
	});

	/* Lambda for handling shell navigation via 'cd'. */
	auto cd = [&proc, &fs](std::vector<std::string> args) -> int32_t
	{
		CLI::App app{"Change present working directory (PWD)"};
		app.allow_windows_style_options(false);

		struct CdArgs
		{
			bool logical_dot_dot{false};
			bool physical_dot_dot{false};
			std::string path{};
		} params{};

		app.add_option("path", params.path, "Directory to move to (full or relative paths)")->required();
		app.add_flag("-L", params.logical_dot_dot, "Handle the operand dot-dot logically");
		app.add_flag("-P", params.physical_dot_dot, "Handle the operand dot-dot physically");
		
		try
		{
			std::ranges::reverse(args);
			args.pop_back();
			app.parse(std::move(args));
		}
		catch(const CLI::ParseError& e)
		{
			int32_t res = app.exit(e, proc.s_out, proc.s_err);
			return res;
		}

		/* Filepath resolution */
		FilePath new_path = FileUtils::resolve(proc, params.path);
		
		if (auto [fid, err] = FileUtils::query(proc, new_path, FileAccessFlags::Execute); err == FileSystemError::Success)
		{
			proc.set_var("PWD", fs->get_path(fid));
			return 0;
		}
		else
		{
			proc.warnln("cd '{}': {}.", new_path, FileSystem::get_fserror_name(err));
			return 1;
		}
	};

	icu::UnicodeString buffer;
	CmdInput::CmdReaderParams read_params;

	proc.set_var("PWD", "/");
	proc.set_var("PATH", "/bin;/usr/bin;/sbin");

	while (true)
	{
		FilePath path(proc.get_var("PWD"));
		std::string_view home_dir = proc.get_var("HOME");

		if (auto [fid, err] = FileUtils::query(proc, path, FileAccessFlags::Read | FileAccessFlags::Execute); err != FileSystemError::Success)
		{
			path = "/";
			proc.set_var("PWD", "/");
		}

		/* A little dirty but the path variable is only used to print the command line,
		so just modify it directly. */
		path.substitute(home_dir, "~");

		std::string usr_str = TermUtils::color(std::format("usr@{}", os.get_hostname()), TermColor::BrightMagenta);
		std::string path_str = TermUtils::color(path.get_string(), TermColor::BrightBlue);
		std::string net_str = (os.get_state() != DeviceState::PoweredOn) ? "(" CSI_CODE(33) "NetBIOS" CSI_RESET ") " : "";

		proc.put("{0}{1}:{2}$ ", net_str, usr_str, path_str);

		auto format = [&proc](const com::CommandQuery& query)
		{
			/* First, update terminal parameters if we're being passed configuration data. */
			if (query.has_screen_data())
			{
				com::ScreenData screen = query.screen_data();
				proc.set_var("TERM_W", screen.size_x());
				proc.set_var("TERM_H", screen.size_y());
			}
		};

		std::string out_cmd = co_await CmdInput::read_cmd_utf8(proc, read_params, format);

		/* At this point we have a valid command -- attempt to execute it. */
		int32_t ret = co_await std::invoke([&proc, &os, &fs, &cd](const std::string& cmd) -> EagerTask<int32_t>
		{
			std::string temp{};
			std::vector<std::string> args{};
			
			std::stringstream ss(cmd);

			while (ss >> std::quoted(temp))
			{
				args.push_back(std::move(temp));
			}

			if (args.empty())
				co_return 1;

			std::string_view name = *std::begin(args);

			if (name == "cd")
				co_return cd(std::move(args));

			for (auto str : proc.get_var("PATH") | std::views::split(';'))
			{
				FilePath prog_path(std::format("{}/{}", std::string_view(str), name));

				if (auto [fid, ptr, err] = FileUtils::open(proc, prog_path, FileAccessFlags::Read | FileAccessFlags::Execute); err == FileSystemError::Success)
				{
					if (auto& prog = ptr->get_executable())
					{
						int32_t ret = co_await os.create_process(prog, std::move(args), &proc);
						co_return ret;
					}
					else
					{
						proc.errln("No program entry point detected!");
						co_return 1;
					}
				}
			}

			proc.warnln("'{0}': No such file or directory.", name);
			co_return 1;

		}, out_cmd);

		if (int32_t term_w = proc.get_var<int32_t>("TERM_W"))
		{
			/* Print a prettier result line if the terminal width parameter is set. */
			auto const current_time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
			std::string ret_sym = std::format(CSI_PLACEHOLDER "{}" CSI_RESET, (ret == 0 ? 32 : 31), (ret == 0 ? "✓" : "✕"));
			std::string time_str = std::format("{:%Y-%m-%d %X}", current_time);
			std::string line_str = std::format("{} {}", ret_sym, time_str);
			int32_t length = 2 + time_str.length();

			proc.putln("\n{}", TermUtils::msg_line(line_str, term_w - length));
		}
		else
		{
			proc.put("\n" CSI_PLACEHOLDER "{} " CSI_RESET, (ret == 0 ? 32 : 31), (ret == 0 ? "✓" : "✕"));
		}

		proc.set_var("RET_VAL", ret);
	}

	co_return 0;
}