#include "os_basic.h"

#include "device.h"
#include "netw.h"
#include "filesystem.h"
#include "navigator.h"

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

	proc.add_reader<CmdSocketAwaiterServer>([sock]() -> std::any
	{
		return sock->async_read();
	});

	proc.add_reader<com::CommandQuery>([sock]() -> std::any
	{
		if (std::optional<com::CommandQuery> opt = sock->read(); opt.has_value())
			return *opt;
		
		return {};
	});

	Navigator nav(*fs);
	proc.set_data(&nav);

	icu::UnicodeString buffer;

	int32_t term_w = 0;
	int32_t term_h = 0;

	while (true)
	{
		FilePath path = nav.get_path();

		proc.set_var("SHELL_PATH", path.get_string());
		proc.set_var("PATH", "/bin;/usr/bin;/sbin");

		std::string usr_str = TermUtils::color(std::format("usr@{}", os.get_hostname()), TermColor::BrightMagenta);
		std::string path_str = TermUtils::color(path.get_string(), TermColor::BrightBlue);
		std::string net_str = (os.get_state() != DeviceState::PoweredOn) ? "(" CSI_CODE(33) "NetBIOS" CSI_RESET ") " : "";

		proc.put("{0}{1}:{2}$ ", net_str, usr_str, path_str);

		while (true)
		{
			com::CommandQuery input = co_await sock->async_read();

			/* First, update terminal parameters if we're being passed configuration data. */
			if (input.has_screen_data())
			{
				com::ScreenData screen = input.screen_data();
				term_w = screen.size_x();
				term_h = screen.size_y();
				proc.set_var("TERM_W", term_w);
				proc.set_var("TERM_H", term_h);
			}

			/* Next, read the actual command and consider it based on first-byte. */
			std::string str_in = input.command();
	
			if (str_in.length() == 0)
				continue;

			if (str_in[0] == '\x1b')
				continue;

			if (str_in[0] == '\x08' || str_in[0] == '\x7f')
			{
				UErrorCode status = U_ZERO_ERROR;
				std::unique_ptr<icu::BreakIterator> bi(icu::BreakIterator::createCharacterInstance(icu::Locale::getDefault(), status));
				if (U_FAILURE(status) || !bi)
				{
					proc.warnln("Warning: Failed to create break iterator!");
					continue;
				}

				bi->setText(buffer);
				int32_t end = buffer.length();
				int32_t last_char_start = bi->preceding(end);
				if (last_char_start != icu::BreakIterator::DONE)
				{
					buffer.remove(last_char_start);
					proc.put(CSI "D" CSI "X");
				}

				continue;
			}

			if (str_in[0] == '\r')
			{
				proc.put("\r\n");
				break;
			}

			if (str_in.back() == '\0')
				str_in.pop_back();

			icu::UnicodeString chunk = icu::UnicodeString::fromUTF8(str_in);
			buffer.append(chunk);
			
			std::string writeback;
			icu::UnicodeString ret_str = chunk.unescape();
			ret_str.toUTF8String(writeback);
			proc.put("{}", writeback);
		}

		/* Trim any whitespace from the buffer string, convert the result back to utf8,
		 and clear the buffer before sending it for processing. */
		std::string out_cmd;
		buffer.trim();
		buffer.toUTF8String(out_cmd);
		buffer.remove();

		/* At this point we have a valid command -- attempt to execute it. */
		int32_t ret = co_await std::invoke([&proc, &os, &fs](const std::string& cmd) -> EagerTask<int32_t>
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

			const std::string& name = *std::begin(args);

			for (auto str : proc.get_var<std::string>("PATH") | std::views::split(';'))
			{
				FilePath prog_path(std::format("{}/{}", std::string_view(str), name));
	
				if (auto [fid, ptr, err] = fs->open(prog_path, FileAccessFlags::Read); err == FileSystemError::Success)
				{
					if (!ptr->has_flag(FileModeFlags::Execute))
					{
						proc.warnln("File '{}' is missing 'execute' flag.", prog_path);
						co_return 1;
					}
	
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

		if (term_w)
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