#include "os_basic.h"

#include "proc.h"

#include "CLI/CLI.hpp"

#include <string>
#include <cstdlib>
#include <print>
#include <iostream>
#include <fstream>

#include <iso646.h>

#include "proto/host.pb.h"
#include "proto/world.pb.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

ProcessTask Programs::CmdLoad(Proc& proc, std::vector<std::string> args)
{
    FileSystem* fs = proc.owning_os->get_filesystem();
	World& world = proc.owning_os->get_world();

	if (fs == nullptr)
	{
		proc.errln("No file system.");
		co_return 2;
	}

	CLI::App app{"An out-of-world utility for restoring the state of the world from a saved file."};
	app.allow_windows_style_options(false);

	struct LoadArgs
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

    if (params.path.is_relative())
	{
        std::error_code err;
		if (params.path = std::filesystem::absolute(params.path, err); err)
        {
            proc.errln("Failed to make path absolute: {}.", err.message());
            co_return 1;
        }
	}

    std::ifstream f(params.path);
    if (not f.is_open())
    {
        std::string err(std::strerror(errno));
        proc.errln("Failed to open input file '{}': {}.", params.path.string(), err);
        co_return 1;
    }

    proc.putln("✓ Opened world file '{}'!", params.path.string());

    world::World in_world;
    google::protobuf::io::IstreamInputStream reader{&f};
    auto status = google::protobuf::json::JsonStreamToMessage(&reader, &in_world);

    if (not status.ok())
    {
        proc.errln("Failed to read world data from file: {}.", status.ToString());
        co_return 1;
    }

    proc.putln("✓ Parsed world data from file!");

    if (not world.deserialize(&in_world))
    {
        proc.errln("Failed to deserialize the world.");
        co_return 1;
    }

    proc.putln("✓ Deserialized world state!");
    proc.putln("✓ World restored!");

    co_return 0;
}