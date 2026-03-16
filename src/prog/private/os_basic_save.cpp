#include "os_basic.h"

#include "proc.h"
#include "world.h"

#include "CLI/CLI.hpp"

#include <string>
#include <cstdlib>
#include <print>
#include <filesystem>
#include <iostream>

#include <iso646.h>

#include "proto/host.pb.h"
#include "proto/world.pb.h"
#include "google/protobuf/json/json.h"


ProcessTask Programs::CmdSave(Proc& proc, std::vector<std::string> args)
{
	FileSystem* fs = proc.owning_os->get_filesystem();
	World& world = proc.owning_os->get_world();

	if (fs == nullptr)
	{
		proc.errln("No file system.");
		co_return 2;
	}

	CLI::App app{"An out-of-world utility for saving the state of the world to your actual computer!"};
	app.allow_windows_style_options(false);

	struct SaveArgs
	{
		std::filesystem::path path{"world.sav"};
	} params{};

	app.add_option("-p,--path,path", params.path, "Location of the saved data");

	try
	{
		std::ranges::reverse(args);
		args.pop_back();
        app.parse(std::move(args));
    }
	catch(const CLI::ParseError& e)
	{
		int res = app.exit(e, proc.s_out, proc.s_err);
        co_return res;
    }

	world::World out_world;
	if (not world.serialize(&out_world))
	{
		proc.errln("Failed to serialize world state.");
		co_return 1;
	}

	proc.putln("✓ Serialized world state!");

	std::string out_str;
	auto status = google::protobuf::json::MessageToJsonString(out_world, &out_str);

	if (not status.ok())
	{
		proc.errln("Failed to collate world data: {}.", status.ToString());
		co_return 1;
	}

	proc.putln("✓ Collated world data!");

	if (params.path.is_relative())
	{
        std::error_code err;
		if (params.path = std::filesystem::absolute(params.path, err); err)
        {
            proc.errln("Failed to make path absolute: {}.", err.message());
            co_return 1;
        }
	}

	std::ofstream f(params.path);
	if (not f)
	{
		std::string err(std::strerror(errno));
		proc.putln("Failed to open world file for writing: {}.", err);
	}

	f << out_str;
	f.close();

	proc.putln("✓ World data successfully written to '{}'!", params.path.string());


	co_return 0;
}