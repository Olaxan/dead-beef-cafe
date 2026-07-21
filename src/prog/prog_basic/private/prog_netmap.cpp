#include "prog_basic.h"

#include "os.h"
#include "net_types.h"
#include "filesystem.h"
#include "addr.h"
#include "net_mgr.h"
#include "race_awaiter.h"

#include "ftxui_host.h"

#include "CLI/CLI.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "proto/ip_packet.pb.h"

#include <string>
#include <vector>
#include <print>
#include <format>
#include <ranges>
#include <functional>
#include <string_view>


ProcessTask Programs::CmdNetMap(Proc& proc, std::vector<std::string> args)
{
	using namespace ftxui;
	using namespace std::chrono_literals;

	size_t term_width = proc.get_var<size_t>("TERM_W");
	size_t term_height = proc.get_var<size_t>("TERM_H");
	float step_size = 1.0f / 16.0f;
	
	bool download = false;
  	bool upload = false;
  	bool ping = false;
 
	auto container = Container::Vertical({
		Checkbox("Download", &download),
		Checkbox("Upload", &upload),
		Checkbox("Ping", &ping),
	});

	FtxuiHost host{container, term_width, term_height};

	co_await host.run(&proc, step_size);

    co_return 0;
}