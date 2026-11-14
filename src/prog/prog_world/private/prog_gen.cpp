#include "prog_world.h"

#include "proc.h"
#include "world.h"
#include "host_utils.h"
#include "os.h"
#include "os_basic.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/app.hpp>             // for Component, App
#include <ftxui/component/captured_mouse.hpp>  // for ftxui
#include <ftxui/component/component.hpp>  // for Slider, Checkbox, Vertical, Renderer, Button, Input, Menu, Radiobox, Toggle
#include <ftxui/component/component_base.hpp>  // for ComponentBase
#include <ftxui/dom/elements.hpp>  // for separator, operator|, Element, size, xflex, text, WIDTH, hbox, vbox, EQUAL, border, GREATER_THAN

#include "proto/gen_params.pb.h"

#include "world_builder.h"
#include "world_data.h"

#include "CLI/CLI.hpp"
#include "ftxui_host.h"

#include <google/protobuf/json/json.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <string>
#include <cstdlib>
#include <print>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>

#include <iso646.h>

using namespace WorldGen;

int32_t ExportSchema(Proc& proc, std::filesystem::path path)
{
	std::ofstream file{path, std::ofstream::out};
	if (not file)
	{
		proc.errln("Failed to open file '{}' for writing.", path.string());
		return 1;
	}

	world::GenParams out_params;
	world::GenCountry& country = *out_params.add_countries();
	country.set_country_name("Example country");
	country.set_primary_business(world::DigitalTechnology);

	world::NamePool& person_names = *country.mutable_person_names();
	person_names.add_prefix("Example given name");
	person_names.add_suffix("Example family name");

	world::NamePool& business_names = *country.mutable_business_names();
	business_names.add_prefix("Example business prefix");
	business_names.add_suffix("Example business suffix");

	world::NamePool& isp_names = *country.mutable_isp_names();
	isp_names.add_prefix("Example ISP prefix");
	isp_names.add_suffix("Example ISP suffix");

	std::string out_str{};
	if (auto status = google::protobuf::json::MessageToJsonString(out_params, &out_str); status.ok())
	{
		file << out_str << '\n';
		if (file)
		{
			proc.putln("Exported schema template to {}.", path.string());
			return 0;
		}
		else
		{
			proc.errln("Failed to write file '{}'.", path.string());
			return 1;
		}
	}
	else
	{
		proc.errln("Failed to write message: {}.", status.ToString());
		return 1;
	}
}

int32_t GenerateWorld(Proc& proc)
{
	return 1;
}

ProcessTask Programs::CmdGen(Proc& proc, std::vector<std::string> args)
{

	CLI::App app{"A program for importing generation data from your real system hard-drive, and using it to create a simulated internet for use within the game."};

	struct GenSharedArgs
	{
		std::filesystem::path path{"assets/gen_params.json"};
	} params{};

	CLI::App* schema_app = app.add_subcommand("schema", "Export a template of the JSON schema");
	CLI::App* import_app = app.add_subcommand("import", "Generate a world");
	
	app.require_subcommand(1);
	
	schema_app->add_option("-f", params.path, "Path to the generation params file (for export)")->capture_default_str();
	import_app->add_option("-f", params.path, "Path to the generation params file (for import)")->capture_default_str();

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

	if (app.got_subcommand(schema_app))
	{
		co_return ExportSchema(proc, params.path);
	}

	if (app.got_subcommand(import_app))
	{
		std::ifstream f{params.path};
		if (not f)
		{
			proc.errln("Failed to open file: '{}'.", params.path.string());
			co_return 1;
		}

		world::GenParams gen_params;
		google::protobuf::io::IstreamInputStream reader{&f};
		auto status = google::protobuf::json::JsonStreamToMessage(&reader, &gen_params);
		if (not status.ok())
		{
			proc.errln("Failed to parse input file: {}.", status.ToString());
			co_return 1;
		}

		World& world = *proc.owning_os->get_outer_as<World*>();

		Uid64 out;
		std::random_device rd;
		std::mt19937 gen(rd());

		auto gen_data = std::make_shared<WorldData>();

		WorldBuilder builder{gen_data};

		std::size_t shared_index{0};

		auto pick_from_string_field = [&](std::string fallback, const google::protobuf::RepeatedPtrField<std::string>& field) -> std::string
		{
			if (field.empty())
				return std::format("{}-{}", fallback, shared_index++);

			std::uniform_int_distribution<> dist(0, field.size() - 1);
			return field.at(dist(gen));
		};

		auto generate_name_from_pool = [&](const world::NamePool& pool, bool use_suffix = true) -> std::string
		{
			if (use_suffix)
			{
				return std::format("{} {}", 
					pick_from_string_field("<INVALID PREFIX>", pool.prefix()),
					pick_from_string_field("<INVALID SUFFIX>", pool.suffix())
				); 
			}
			else
			{
				return pick_from_string_field("<INVALID PREFIX>", pool.prefix());
			}
		};

		std::uniform_int_distribution<std::size_t> distribution_regional_isp(4, 8);
		std::uniform_int_distribution<std::size_t> distribution_100(0, 100);

		auto generate_country = [&](const world::GenCountry& country, Uid64 upstream)
		{
			auto generate_isp_name = [&]() -> std::string
			{
				bool has_suffix = (distribution_100(gen) > 40);

				std::string res = (country.has_isp_names())
					? generate_name_from_pool(country.isp_names(), has_suffix)
					: std::format("ISP-{}", shared_index);
				
				proc.putln(" ├── Generated '{}'.", res);
				return std::move(res);
			};

			ClusterParams cluster
			{
				.number_of_hosts = distribution_regional_isp(gen),
				.hostname_generator = generate_isp_name
			};

			proc.putln("Generating '{}' with {} ISP(s)...", country.country_name(), cluster.number_of_hosts);
			std::string cluster_name = std::format("{} Regional Gateway", country.country_name());

			Cluster& isp_cluster = builder.generate_cluster(cluster_name, upstream, cluster);
		};


		Uid64 hub = builder.add_host("Central Routing Node");
		
		for (auto&& country : gen_params.countries())
		{
			int32_t num_isp = distribution_regional_isp(gen);
			generate_country(country, hub);
		}

		proc.putln("Imported data succesfully.");
	}

	co_return 0;
}
