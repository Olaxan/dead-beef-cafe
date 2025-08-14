#include "os_basic.h"

#include "filesystem.h"
#include "navigator.h"
#include "os_input.h"
#include "editor.h"

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
#include <cctype>


EagerTask<com::CommandQuery> await_input(Proc& proc)
{
	OS& os = *proc.owning_os;

	if (auto async_read = proc.read<CmdSocketAwaiterServer>())
	{
		co_return (co_await *async_read);
	}
	else
	{
		while (true)
		{
			if (std::optional<com::CommandQuery> opt_input = proc.read<com::CommandQuery>())
				co_return *opt_input;

			co_await os.wait(0.16f);
		}
	}
}

EagerTask<std::optional<std::string>> write_in_statusbar(EditorState& state, Proc& proc, std::string prefix)
{
	OS& os = *proc.owning_os;

	EditorState substate{25, 1, false};
	substate.init_state();

	state.set_status(std::format("{}: {}", prefix, substate.as_utf8()));
	proc.put("{}", state.get_printout());

	while (true)
	{
		com::CommandQuery query_in = co_await await_input(proc);
		std::string str_in = query_in.command();
		EditorState::HandlerReturn ret = substate.accept_input(str_in);

		if (ret == EditorState::HandlerReturn::WantExit)
		{
			state.set_status("Save aborted.");
			co_return std::nullopt;
		}

		if (ret == EditorState::HandlerReturn::Return)
		{
			break;
		}

		state.set_status(std::format("{}: {}", prefix, substate.as_utf8()));
		proc.put("{}", state.get_printout());
	}

	state.reset_status();
	co_return substate.as_utf8();
}

EagerTask<bool> handle_save(EditorState& state, Proc& proc);
EagerTask<bool> handle_save_as(EditorState& state, Proc& proc)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();

	if (std::optional<std::string> name = co_await write_in_statusbar(state, proc, "Save as"))
	{
		FilePath new_path(*name);
		if (new_path.is_relative())
			new_path.prepend(proc.get_var("SHELL_PATH"));

		state.set_path(new_path);

		co_return (co_await handle_save(state, proc));
	}

	co_return false;
}

EagerTask<bool> handle_save(EditorState& state, Proc& proc)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();

	if (auto opt_path = state.get_path(); opt_path.has_value())
	{
		auto [fid, ptr, err] = fs->open(*opt_path);
		if (err == FileSystemError::Success)
		{
			ptr->write(state.as_utf8());
			state.set_dirty(0);
			co_return true;
		}
	}

	co_return (co_await handle_save_as(state, proc));

}

ProcessTask Programs::CmdEdit(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No filesystem!");
		co_return 1;
	}

	int32_t term_w = proc.get_var<int32_t>("TERM_W");
	int32_t term_h = proc.get_var<int32_t>("TERM_H");

	if (term_w < 20 || term_h < 10)
	{
		proc.warnln("The terminal window is too small, or 'TERM_W'/'TERM_H' wasn't set.");
		co_return 1;
	}

	EditorState state{term_w, term_h};

	if (args.size() > 1)
	{
		FilePath path(args[1]);

		if (path.is_relative())
			path.prepend(proc.get_var("SHELL_PATH"));

		if (auto [fid, f, err] = fs->open(path); err == FileSystemError::Success) 
		{
			//state.set_file(path, f);
		}
	}

	com::CommandReply begin_msg;
	begin_msg.set_reply(std::format(BEGIN_ALT_SCREEN_BUFFER "{}", state.get_printout()));
	proc.write(begin_msg);

	while (true)
	{
		com::CommandQuery input = co_await await_input(proc);

		/* First, update terminal parameters if we're being passed configuration data. */
		if (input.has_screen_data())
		{
			com::ScreenData screen = input.screen_data();
			state.set_size(screen.size_x(), screen.size_y());
		}

		/* Next, read the actual command and consider it based on first-byte. */
		std::string str_in = input.command();

		for (char c : str_in)
			std::print("{}, ", (int)c);
		std::println("was received.");

		EditorState::HandlerReturn ret = state.accept_input(str_in);

		if (ret == EditorState::HandlerReturn::WantExit)
		{
			if (!state.is_dirty())
				break;

			state.set_status("The file is unsaved (Ctrl+S to save).");
		}

		if (ret == EditorState::HandlerReturn::WantSave)
		{
			co_await handle_save(state, proc);
		}		

		proc.put("{}", state.get_printout());
	}

	com::CommandReply end_msg;
	end_msg.set_reply(END_ALT_SCREEN_BUFFER "Goodbye...\n");
	proc.write(end_msg);

	co_return 0;
}
