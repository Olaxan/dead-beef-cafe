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

	void resize(size_t width, size_t height);

	bool feed_event(const ftxui::Event& event);

	bool feed_command_bytes(std::string_view command);

	bool feed_query(const com::CommandQuery& query);

	std::string ansi_output() const
	{
		return last_frame_;
	}

	com::CommandReply make_reply() const;

	EagerTask<int32_t> run(Proc* proc, float refresh_rate = 1.0f);

	void refresh();

private:

	void ensure_screen();

	Proc* proc_{nullptr};
	ftxui::Component root_;
	ftxui::TerminalInputParser terminal_input_parser_;
	std::shared_ptr<ftxui::Screen> screen_{nullptr};
	std::string last_frame_;
	size_t width_{80};
	size_t height_{24};

};