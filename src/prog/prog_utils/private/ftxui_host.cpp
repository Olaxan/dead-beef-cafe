#include "ftxui_host.h"
#include "proc.h"
#include "race_awaiter.h"

FtxuiHost::FtxuiHost(ftxui::Component root, size_t width, size_t height)
	: root_(std::move(root)), terminal_input_parser_([&](ftxui::Event event)
	{
		feed_event(std::move(event));
	})
{
	resize(width, height);
}

void FtxuiHost::set_root_component(ftxui::Component root)
{
	root_ = std::move(root);
	if (root_)
	{
		root_->TakeFocus();
	}
	refresh();
}

void FtxuiHost::install()
{
	if (use_alternate_screen_)
	{
		proc_->put(BEGIN_ALT_SCREEN_BUFFER);
	}

	if (hide_cursor_)
	{
		proc_->put(HIDE_CURSOR);
	}

	if (track_mouse_)
	{
		proc_->put(CSI "?1000h");
		proc_->put(CSI "?1003h");
		proc_->put(CSI "?1015h");
		proc_->put(CSI "?1006h");
	}
}

void FtxuiHost::uninstall()
{
	if (use_alternate_screen_)
	{
		proc_->put(END_ALT_SCREEN_BUFFER);
	}

	if (hide_cursor_)
	{
		proc_->put(SHOW_CURSOR);
	}

	if (track_mouse_)
	{
		proc_->put(CSI "?1000l");
		proc_->put(CSI "?1003l");
		proc_->put(CSI "?1015l");
		proc_->put(CSI "?1006l");
	}
}

void FtxuiHost::resize(size_t width, size_t height)
{
	width = width > 0 ? width : 1;
	height = height > 0 ? height : 1;
	if (width_ != width || height_ != height || !screen_)
	{
		width_ = width;
		height_ = height;
		screen_ = std::make_shared<ftxui::Screen>(width, height);
		refresh();
	}
}

bool FtxuiHost::feed_event(ftxui::Event&& event)
{
	if (!root_)
	{
		return false;
	}

	if (event.is_mouse())
	{
		/* This likely doesn't always hold true,
		as in the FTXUI App.cpp the cursor position is
		subtracted instead of a constant value. */
		event.mouse().x += -1;
		event.mouse().y += -1;
	}

	ensure_screen();
	root_->TakeFocus();
	const bool handled = root_->OnEvent(event);
	refresh();
	return handled;
}

bool FtxuiHost::feed_command_bytes(std::string_view command)
{
	if (command.empty())
	{
		return false;
	}

	for (char c : command)
	{
		terminal_input_parser_.Add(c);
	}

	return true;
}

bool FtxuiHost::feed_query(const com::CommandQuery& query)
{
	if (query.has_screen_data())
	{
		resize(query.screen_data().size_x(), query.screen_data().size_y());
	}

	return feed_command_bytes(query.command());
}

com::CommandReply FtxuiHost::make_reply() const
{
	com::CommandReply reply;
	reply.set_reply(last_frame_);
	return reply;
}

EagerTask<int32_t> FtxuiHost::run(Proc* proc, float refresh_rate)
{
	assert(proc);

	proc_ = proc;

	install();
	refresh();

	while (true)
	{
		proc_->write(last_frame_);
		
		auto exp_read = co_await when_any(proc_->io.read_query(), proc_->wait(refresh_rate));
		if (exp_read.index == 0)
		{
			if (auto res = std::get<1>(exp_read.value))
			{
				feed_query(*res);
			}
			else
			{
				break;
			}
		}

		refresh();
	}

	uninstall();

	co_return 0;
}

void FtxuiHost::refresh()
{
	if (!root_)
	{
		last_frame_.clear();
		return;
	}

	ensure_screen();
	screen_->Clear();
	ftxui::Render(*screen_, root_->Render());
	last_frame_ = screen_->ToString();
	last_frame_ += screen_->ResetPosition(false);
}

void FtxuiHost::ensure_screen()
{
	if (!screen_)
	{
		resize(80, 24);
	}
}
