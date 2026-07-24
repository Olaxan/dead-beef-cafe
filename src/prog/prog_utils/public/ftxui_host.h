#pragma once

#include "task.h"

#include "proto/query.pb.h"
#include "proto/reply.pb.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/terminal_input_parser.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

class Proc;
class FtxuiHost
{
public:

	FtxuiHost() = delete;

	FtxuiHost(ftxui::Component root, size_t width = 80, size_t height = 24);

	void set_root_component(ftxui::Component root);

	ftxui::Component root_component() const
	{
		return root_;
	}

	void install();
	void uninstall();

	void resize(size_t width, size_t height);

	bool feed_event(ftxui::Event&& event);

	bool feed_command_bytes(std::string_view command);

	bool feed_query(const com::CommandQuery& query);

	std::string ansi_output() const
	{
		return last_frame_;
	}

	com::CommandReply make_reply() const;

	EagerTask<int32_t> run(Proc* proc, float refresh_rate = 1.0f);

	void refresh();

	void set_use_alternate_screen(bool use_alternate_screen)
	{
		use_alternate_screen_ = use_alternate_screen;
	}

	void set_track_mouse(bool track_mouse)
	{
		track_mouse_ = track_mouse;
	}

private:

	void ensure_screen();

	bool track_mouse_{true};
	bool use_alternate_screen_{true};
	bool hide_cursor_{true};

	Proc* proc_{nullptr};
	ftxui::Component root_;
	ftxui::TerminalInputParser terminal_input_parser_;
	std::shared_ptr<ftxui::Screen> screen_{nullptr};
	std::string last_frame_;
	std::string current_frame_;
	std::chrono::steady_clock::time_point last_write_time_{std::chrono::steady_clock::now()};
	size_t width_{80};
	size_t height_{24};

};