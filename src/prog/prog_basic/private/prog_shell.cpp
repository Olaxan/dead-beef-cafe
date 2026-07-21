#include "prog_basic.h"

#include "os.h"
#include "device.h"
#include "net_types.h"
#include "filesystem.h"
#include "race_awaiter.h"
#include "sync_awaiter.h"
#include "sync_awaiter_dynamic.h"

#include "CLI/CLI.hpp"

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/usearch.h>
#include <unicode/ustring.h>
#include <unicode/ustream.h>
#include <unicode/brkiter.h>

#include <csignal>
#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <ranges>
#include <array>
#include <functional>

using SubCmdRange = std::ranges::split_view<std::string_view, std::ranges::single_view<char>>;
using ArgList = std::vector<std::string>;


// trim from left
inline std::string_view ltrim(std::string_view s)
{
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
		s.remove_prefix(1);
    
	return s;
}

// trim from right
inline std::string_view rtrim(std::string_view s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
		s.remove_suffix(1);
    
	return s;
}

// trim from left & right
inline std::string_view trim(std::string_view s)
{
    return ltrim(rtrim(s));
}

/* For handling shell navigation via 'cd'. */
auto CmdCd(Proc& proc, std::vector<std::string> args) -> Task<int32_t>
{
	FileSystem* fs = proc.owning_os->get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No file system.");
		co_return 1;
	}

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
		co_return res;
	}

	/* Filepath resolution */
	FilePath new_path = proc.fs.resolve(params.path);
	
	if (auto exp_fid = proc.fs.query(new_path, FileAccessFlags::Execute))
	{
		proc.set_var("PWD", fs->get_path(*exp_fid));
		co_return 0;
	}
	else
	{
		proc.warnln("cd '{}': {}.", new_path, exp_fid.error().message());
		co_return 1;
	}
};

Task<int32_t> ProcessSubCmdSingle(Proc& proc, std::string_view cmd, bool background = false)
{
	ArgList args = proc.sys.make_args(cmd);

	if (args.empty())
		co_return 1;

	std::string_view name = *args.begin();
	auto exp_path = proc.sys.find_in_path(name);

	if (not exp_path)
	{
		proc.warnln("'{}': {}.", name, exp_path.error().message());
		co_return 1;
	}

	co_return (co_await proc.sys.exec(*exp_path, std::move(args)));
}

Task<int32_t> ProcessSubCmdPipeline(Proc& proc, SubCmdRange& cmds, bool background = false)
{
	using Pipe = MessageQueue<WriteInput>;

	FileSystem* fs = proc.owning_os->get_filesystem();

	std::size_t num_tasks = std::ranges::distance(cmds);
	std::size_t num_pipes = num_tasks - 1;

	std::vector<Pipe> pipes(num_pipes);
	std::vector<Task<int32_t>> jobs;
	jobs.reserve(num_tasks);

	std::size_t idx = 0;
	for (auto&& subcmd : cmds)
	{
		std::string_view cmd_sv{subcmd};
		trim(cmd_sv);

		ArgList args = proc.sys.make_args(cmd_sv);

		if (args.empty())
			co_return 1;

		std::string_view name = *args.begin();

		if (name.compare("cd") == 0)
		{
			co_return (co_await CmdCd(proc, args));
		}

		auto exp_path = proc.sys.find_in_path(name);

		if (not exp_path)
		{
			proc.warnln("'{}': {}.", name, exp_path.error().message());
			co_return 1;
		}

		ExecParams params;
		params.run_in_background = background;
		params.is_tty = (idx == 0);

		/* Unless this is the first program in the pipeline, read from the pipe. */
		if (idx > 0)
		{
			params.reader = [pipe = &pipes[idx - 1]](Proc& rproc) -> Task<ReadResult>
			{
				auto res = co_await when_any(pipe->async_pop(), rproc.await_signal());
				if (res.index == 0)
				{
					if (auto msg = std::get<1>(res.value); msg.has_value())
					{
						co_return msg.value();
					}
					else
					{
						rproc.signal(SIGTERM);
						co_return std::unexpected{msg.error()};
					}
				}
				else
				{
					co_return std::unexpected{std::error_condition{EPIPE, std::generic_category()}};
				}
			};
		}

		/* Unless this is the last program in the pipeline, write to the pipe. */
		if (idx < num_pipes)
		{
			params.writer = [pipe = &pipes[idx]](Proc& wproc, WriteInput str)
			{
				pipe->push(std::move(str));
			};
		}

		Task<int32_t> job = proc.sys.exec(*exp_path, std::move(args), std::move(params));
		jobs.push_back(std::move(job));
		
		++idx;
	}

	auto res = co_await when_all_dynamic(std::move(jobs));

	co_return std::ranges::max(res);
}

ProcessTask Programs::CmdShell(Proc& proc, std::vector<std::string> args)
{
	using namespace std::string_view_literals;

	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();
	UsersManager* users = os.get_users_manager();

	if (fs == nullptr)
	{
		proc.errln("No file system!");
		co_return 1;
	}

	icu::UnicodeString buffer;
	CmdReaderParams read_params;

	if (proc.get_var("PWD").empty())
		proc.set_var("PWD", "/");
		
	proc.set_var("PATH", "/bin;/usr/bin;/sbin");

	while (true)
	{
		FilePath path(proc.get_var("PWD"));
		std::string_view home_dir = proc.get_var("HOME");

		if (not proc.fs.query(path, FileAccessFlags::Read | FileAccessFlags::Execute))
		{
			path = "/";
			proc.set_var("PWD", "/");
		}

		/* A little dirty but the path variable is only used to print the command line,
		so just modify it directly. */
		path.substitute(home_dir, "~");

		int32_t uid = proc.get_uid();
		int32_t gid = proc.get_gid();

		std::optional<std::string_view> username = users->get_username(uid);

		std::string usr_str = std::format(CSI_CODE(95) "{}@{}" CSI_RESET, username.value_or("-"), os.get_hostname());
		std::string path_str = std::format(CSI_CODE(94) "{}" CSI_RESET, path.get_string());
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

		auto exp_out_cmd = co_await proc.io.read_cmd_utf8(read_params, format);

		if (not exp_out_cmd)
		{
			proc.putln("Read failure.");
			co_return 1;
		}

		std::string_view out_cmd{*exp_out_cmd};
		trim(out_cmd);

		if (out_cmd.empty())
		{
			continue;
		}

		if (out_cmd.compare("exit") == 0)
		{
			proc.putln("Goodbye...");
			co_return 0;
		}

		bool background = std::invoke([&]() -> bool
		{
			if (out_cmd.empty()) 
				return false;

			if (out_cmd.back() == '&')
			{
				out_cmd.remove_suffix(1);
				return true;
			}
			
			return false;
		});

		auto subcmd = std::views::split(out_cmd, '|');
		std::size_t num_subcmd = std::ranges::distance(subcmd);

		/* At this point we have a valid command -- attempt to execute it. */
		int32_t ret = co_await ProcessSubCmdPipeline(proc, subcmd, background);

		/* --- COMMAND DONE, print status line --- */

		if (int32_t term_w = proc.get_var<int32_t>("TERM_W"))
		{
			/* Print a prettier result line if the terminal width parameter is set. */
			auto const current_time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
			std::string ret_sym = std::format(CSI_PLACEHOLDER "{}" CSI_RESET, (ret == 0 ? 32 : 31), (ret == 0 ? "✓" : "✕"));
			std::string time_str = std::format("{:%Y-%m-%d %X}", current_time);
			std::string line_str = std::format("{} {}", ret_sym, time_str);
			int32_t length = static_cast<int32_t>(2 + time_str.length());

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