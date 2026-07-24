#include "prog_basic.h"

#include "ftxui_host.h"

#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <cctype>

#include "markdown/parser.hpp"
#include "markdown/viewer.hpp"
#include "markdown/editor.hpp"

#include <ftxui/component/app.hpp>

#include <iso646.h>

ProcessTask Programs::CmdMarkdown(Proc& proc, std::vector<std::string> args)
{

	size_t term_width = proc.get_var<size_t>("TERM_W");
	size_t term_height = proc.get_var<size_t>("TERM_H");
	float step_size = 1.f;

	int32_t split_size = term_width / 2;
	int32_t min_split_size = 20;
	int32_t max_split_size = term_width - 30;

	auto editor = std::make_shared<markdown::Editor>();
	editor->set_content("# My Document\n\nStart typing...");

	auto viewer = std::make_shared<markdown::Viewer>(
		markdown::make_cmark_parser());

	// Sync scroll to cursor position:
	float ratio = 0.0f;
	if (editor->total_lines() > 1) {
		ratio = static_cast<float>(editor->cursor_line() - 1) /
				static_cast<float>(editor->total_lines() - 1);
	}
	viewer->set_scroll(ratio);
	viewer->show_scrollbar(true);

	auto split = ftxui::ResizableSplit({
		.main = editor->component(),
		.back = viewer->component(),
		.direction = ftxui::Direction::Left,
		.main_size = &split_size,
		.min = &min_split_size,
		.max = &max_split_size,
	});

	auto renderer = ftxui::Renderer(split, [&] {
		viewer->set_content(editor->content());  // Live update
		return split->Render() | ftxui::flex | ftxui::borderLight;
	});

	FtxuiHost host{renderer, term_width, term_height};

	co_return (co_await host.run(&proc, step_size));
}