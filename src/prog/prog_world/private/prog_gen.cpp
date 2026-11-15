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

#include "gen_params_generated.h"

#include "world_builder.h"
#include "world_data.h"

#include "CLI/CLI.hpp"
#include "ftxui_host.h"

#include <flatbuffers/idl.h>

#include <algorithm>
#include <string>
#include <cstdlib>
#include <print>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>
#include <iterator>
#include <stdexcept>

#include <iso646.h>

using namespace WorldGen;

template<typename T>
const T& get_random_from_vector(std::mt19937& gen, std::vector<T>& v)
{
	assert(v.size());
	std::uniform_int_distribution<std::size_t> r{0, v.size() - 1};
	std::size_t idx = r(gen);
	return v.at(idx);
}

void print_generated_modules(
	Proc& proc,
	const std::vector<GeneratedModule>& modules,
	std::size_t depth)
{
	const std::string indentation(depth * 2, ' ');
	for (const GeneratedModule& module : modules)
	{
		proc.putln(
			"{}- {}: {} ({}/{}) [{} host(s)]",
			indentation,
			module.name,
			module.hostname,
			module.address,
			static_cast<unsigned int>(module.network_prefix),
			module.hosts.size());
		print_generated_modules(proc, module.children, depth + 1);
	}
}

int32_t ExportSchema(Proc& proc, std::filesystem::path path)
{
	std::ofstream file{path, std::ofstream::out};
	if (not file)
	{
		proc.errln("Failed to open file '{}' for writing.", path.string());
		return 1;
	}

	flatbuffers::FlatBufferBuilder builder;
	auto make_name_pool = [&](const char* prefix, const char* suffix)
	{
		const std::vector<flatbuffers::Offset<flatbuffers::String>> prefixes{builder.CreateString(prefix)};
		const std::vector<flatbuffers::Offset<flatbuffers::String>> suffixes{builder.CreateString(suffix)};
		return world::CreateNamePool(builder, builder.CreateVector(prefixes), builder.CreateVector(suffixes));
	};

	const auto person_names = make_name_pool("Example given name", "Example family name");
	const auto business_names = make_name_pool("Example business prefix", "Example business suffix");
	const auto isp_names = make_name_pool("Example ISP prefix", "Example ISP suffix");
	const auto country = world::CreateGenCountryDirect(
		builder,
		"Example country",
		world::BusinessType_DigitalTechnology,
		person_names,
		business_names,
		isp_names);

	const auto household_module = world::CreateGenModuleDirect(
		builder, "Household", 2u, 5u, 1u, 6u, 0, nullptr, 64u);
	const auto corporation_module = world::CreateGenModuleDirect(
		builder, "Corporation", 1u, 3u, 3u, 10u, 0, nullptr, 32u);
	const std::vector<flatbuffers::Offset<world::GenModule>> isp_children{
		household_module,
		corporation_module
	};
	const auto isp_module = world::CreateGenModuleDirect(
		builder,
		"ISP",
		4u,
		8u,
		flatbuffers::nullopt,
		flatbuffers::nullopt,
		0,
		&isp_children,
		16u);
	const std::vector<flatbuffers::Offset<world::GenCountry>> countries{country};
	const std::vector<flatbuffers::Offset<world::GenModule>> modules{isp_module};
	const auto out_params = world::CreateGenParamsDirect(builder, &countries, &modules);
	world::FinishGenParamsBuffer(builder, out_params);

	flatbuffers::Parser json_parser;
	if (not json_parser.Deserialize(world::GenParams::BinarySchema::data(), world::GenParams::BinarySchema::size()))
	{
		proc.errln("Failed to load FlatBuffers schema: {}.", json_parser.error_);
		return 1;
	}

	std::string out_str{};
	if (const char* error = flatbuffers::GenerateText(json_parser, builder.GetBufferPointer(), &out_str))
	{
		proc.errln("Failed to serialize schema template: {}.", error);
		return 1;
	}

	file << out_str << '\n';
	if (not file)
	{
		proc.errln("Failed to write file '{}'.", path.string());
		return 1;
	}

	proc.putln("Exported schema template to {}.", path.string());
	return 0;
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

		const std::string json{
			std::istreambuf_iterator<char>{f},
			std::istreambuf_iterator<char>{}
		};

		if (f.bad())
		{
			proc.errln("Failed to read input file: '{}'.", params.path.string());
			co_return 1;
		}

		flatbuffers::Parser json_parser;
		if (not json_parser.Deserialize(world::GenParams::BinarySchema::data(), world::GenParams::BinarySchema::size()))
		{
			proc.errln("Failed to load FlatBuffers schema: {}.", json_parser.error_);
			co_return 1;
		}

		json_parser.opts.strict_json = true;

		if (not json_parser.ParseJson(json.c_str()))
		{
			proc.errln("Failed to parse input file: {}.", json_parser.error_);
			co_return 1;
		}

		const world::GenParams* gen_params = world::GetGenParams(json_parser.builder_.GetBufferPointer());

		proc.putln("Successfully read generation data from {}.", params.path.string());
		proc.putln("Generating world...");

		World& outer_world = *proc.owning_os->get_outer_as<World*>();

		auto gen_data = std::make_shared<WorldData>();
		WorldBuilder builder{gen_data};

		std::size_t shared_index{0};

		std::random_device rd;
		std::mt19937 gen(rd());

		auto pick_from_string_field = [&](std::string fallback, const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>* field) -> std::string
		{
			if (field == nullptr or field->empty())
				return std::format("{}-{}", fallback, shared_index++);

			std::uniform_int_distribution<> dist(0, field->size() - 1);
			return field->Get(dist(gen))->str();
		};

		auto generate_name_from_pool = [&](const world::NamePool& pool, const std::string& fallback) -> std::string
		{
			std::string prefix = pick_from_string_field(fallback, pool.prefix());
			if (pool.suffix() == nullptr or pool.suffix()->empty())
				return prefix;

			return std::format("{} {}", prefix, pick_from_string_field(fallback, pool.suffix()));
		};

		auto has_names = [](const world::NamePool& pool)
		{
			return (pool.prefix() != nullptr and not pool.prefix()->empty())
				or (pool.suffix() != nullptr and not pool.suffix()->empty());
		};

		auto get_string = [](const flatbuffers::String* value)
		{
			return value == nullptr ? std::string{} : value->str();
		};

		auto default_network_prefix = [](const std::string& module_name, uint8_t parent_prefix) -> uint32_t
		{
			if (module_name == "ISP")
				return 16;
			if (module_name == "Corporation")
				return 32;
			if (module_name == "Household")
				return 64;
			return std::min<uint32_t>(static_cast<uint32_t>(parent_prefix) + 16, 127);
		};

		std::function<bool(const world::GenModule&, uint8_t)> module_is_valid =
			[&](const world::GenModule& module, uint8_t parent_prefix)
		{
			const std::string module_name = get_string(module.moduleName());
			if (module_name.empty())
			{
				proc.errln("Generation modules must have a moduleName.");
				return false;
			}
			const uint32_t min_instances = module.minInstances().value_or(1);
			const uint32_t max_instances = module.maxInstances().value_or(min_instances);
			const uint32_t min_hosts = module.minHosts().value_or(0);
			const uint32_t max_hosts = module.maxHosts().value_or(min_hosts);
			const uint32_t network_prefix = module.networkPrefix().value_or(
				default_network_prefix(module_name, parent_prefix));

			if (min_instances > max_instances)
			{
				proc.errln("Generation module '{}' has minInstances greater than maxInstances.", module_name);
				return false;
			}

			if (min_hosts > max_hosts)
			{
				proc.errln("Generation module '{}' has minHosts greater than maxHosts.", module_name);
				return false;
			}

			if (network_prefix <= parent_prefix || network_prefix > 127)
			{
				proc.errln("Generation module '{}' must have a networkPrefix between {} and 127.", module_name, parent_prefix + 1);
				return false;
			}

			if (const auto* children = module.children())
			{
				for (const world::GenModule* child : *children)
				{
					if (not module_is_valid(*child, static_cast<uint8_t>(network_prefix)))
						return false;
				}
			}

			return true;
		};

		if (const auto* modules = gen_params->modules())
		{
			for (const world::GenModule* module : *modules)
			{
				if (not module_is_valid(*module, 8))
					co_return 1;
			}
		}

		GenerationPlan plan;
		std::function<GenerationModule(const world::GenModule&, const world::GenCountry&, uint8_t)> convert_module;
		convert_module = [&](const world::GenModule& source, const world::GenCountry& country, uint8_t parent_prefix)
		{
			GenerationModule module;
			module.name = get_string(source.moduleName());
			module.min_instances = source.minInstances().value_or(1);
			module.max_instances = source.maxInstances().value_or(module.min_instances);
			module.min_hosts = source.minHosts().value_or(0);
			module.max_hosts = source.maxHosts().value_or(module.min_hosts);
			module.network_prefix = static_cast<uint8_t>(source.networkPrefix().value_or(
				default_network_prefix(module.name, parent_prefix)));

			const world::NamePool* name_pool = nullptr;
			if (source.namePool() != nullptr)
				name_pool = source.namePool();
			else if (module.name == "ISP")
				name_pool = country.ispNames();
			else if (module.name == "Corporation")
				name_pool = country.businessNames();
			else if (module.name == "Household")
				name_pool = country.personNames();

			if (name_pool != nullptr and has_names(*name_pool))
			{
				module.name_generator = [&, name_pool, fallback = module.name]()
				{
					return generate_name_from_pool(*name_pool, fallback);
				};
			}

			if (const auto* children = source.children())
			{
				for (const world::GenModule* child : *children)
					module.children.emplace_back(convert_module(*child, country, module.network_prefix));
			}

			return module;
		};

		if (const auto* countries = gen_params->countries())
		{
			for (const world::GenCountry* country : *countries)
			{
				GenerationModule country_module;
				country_module.name = "Country";
				country_module.network_prefix = 8;
				country_module.name_generator = [country, get_string]()
				{
					return get_string(country->countryName());
				};
				if (const auto* modules = gen_params->modules())
				{
					for (const world::GenModule* module : *modules)
						country_module.children.emplace_back(convert_module(*module, *country, country_module.network_prefix));
				}

				if (gen_params->modules() == nullptr or gen_params->modules()->empty())
				{
					GenerationModule fallback;
					fallback.name = "ISP";
					fallback.min_instances = 4;
					fallback.max_instances = 8;
					fallback.min_hosts = 4;
					fallback.max_hosts = 8;
					fallback.network_prefix = 16;
					const world::NamePool* name_pool = country->ispNames();
					if (name_pool != nullptr and has_names(*name_pool))
					{
						fallback.name_generator = [&, name_pool]()
						{
							return generate_name_from_pool(*name_pool, "ISP");
						};
					}
					country_module.children.emplace_back(std::move(fallback));
				}
				plan.modules.emplace_back(std::move(country_module));
			}
		}

		Uid64 hub = builder.generate_host("Central Routing Node");
		std::vector<GeneratedModule> generated;
		try
		{
			generated = builder.execute(plan, hub, gen);
		}
		catch (const std::invalid_argument& error)
		{
			proc.errln("Invalid generation plan: {}.", error.what());
			co_return 1;
		}
		catch (const std::length_error& error)
		{
			proc.errln("Generation plan exceeds its address space: {}.", error.what());
			co_return 1;
		}
		catch (const std::overflow_error& error)
		{
			proc.errln("Generated address overflow: {}.", error.what());
			co_return 1;
		}

		std::vector<GeneratedModule*> isps;
		std::function<void(std::vector<GeneratedModule>&)> collect_isps =
			[&](std::vector<GeneratedModule>& modules)
		{
			for (GeneratedModule& module : modules)
			{
				if (module.name == "ISP")
					isps.emplace_back(&module);
				collect_isps(module.children);
			}
		};
		collect_isps(generated);

		if (isps.empty())
		{
			proc.errln("No new ISPs were created, so the existing host cannot be migrated.");
			co_return 1;
		}
		GeneratedModule* target_isp = get_random_from_vector(gen, isps);

		auto hub_address_result = Address6::from_string("fd00::1");
		if (not hub_address_result)
		{
			proc.errln("Failed to assign the central routing node address.");
			co_return 1;
		}
		const Address6 hub_address = *hub_address_result;

		auto configure_host = [&](Uid64 uid, Address6 address) -> NetManager*
		{
			auto it = gen_data->hosts.find(uid);
			if (it == gen_data->hosts.end() or not it->second)
			{
				proc.errln("Generated host '{}' was not found.", uid);
				return nullptr;
			}

			Host& host = *it->second;
			HostUtils::prepare_host<BasicOS>(outer_world, host);
			NetManager* net = host.get_os().get_network_manager();
			if (net == nullptr)
			{
				proc.errln("Generated host '{}' has no network manager.", uid);
				return nullptr;
			}

			net->set_primary_ip(address);
			return net;
		};

		NetManager* hub_network = configure_host(hub, hub_address);
		if (hub_network == nullptr)
			co_return 1;

		std::function<bool(const GeneratedModule&, Address6)> configure_module_network =
			[&](const GeneratedModule& module, Address6 upstream_address)
		{
			NetManager* net = configure_host(module.root, module.address);
			if (net == nullptr)
				return false;

			for (const GeneratedModule& child : module.children)
			{
				net->add_route(child.subnet, child.network_prefix, child.address, RouteType::Static);
			}
			net->add_route(Address6{}, 0, upstream_address, RouteType::Static);

			for (const GeneratedHost& host : module.hosts)
			{
				NetManager* host_network = configure_host(host.uid, host.address);
				if (host_network == nullptr)
					return false;
				host_network->add_route(Address6{}, 0, module.address, RouteType::Static);
			}

			for (const GeneratedModule& child : module.children)
			{
				if (not configure_module_network(child, module.address))
					return false;
			}
			return true;
		};

		for (const GeneratedModule& country : generated)
		{
			hub_network->add_route(country.subnet, country.network_prefix, country.address, RouteType::Static);
			if (not configure_module_network(country, hub_address))
				co_return 1;
		}

		const Address6 player_address = target_isp->next_host_address;
		if (not NetManager::same_subnet(player_address, target_isp->subnet, target_isp->network_prefix))
		{
			proc.errln("The selected ISP has no available address for this host.");
			co_return 1;
		}

		NetManager* player_network = proc.owning_os->get_network_manager();
		if (player_network == nullptr)
		{
			proc.errln("The current host has no network manager.");
			co_return 1;
		}

		player_network->set_primary_ip(player_address);
		player_network->add_route(Address6{}, 0, target_isp->address, RouteType::Static);

		auto target_isp_host = gen_data->hosts.find(target_isp->root);
		if (target_isp_host == gen_data->hosts.end() or not target_isp_host->second)
		{
			proc.errln("Selected ISP '{}' was not found.", target_isp->hostname);
			co_return 1;
		}

		NetManager* target_isp_network = target_isp_host->second->get_os().get_network_manager();

		if (target_isp_network == nullptr)
		{
			proc.errln("Selected ISP '{}' has no network manager.", target_isp->hostname);
			co_return 1;
		}

		target_isp_network->add_route(player_address, 128, player_address, RouteType::Static);

		gen_data->links->relate(target_isp->root, proc.owning_os->get_uid());
		target_isp->hosts.emplace_back(GeneratedHost{
			.uid = proc.owning_os->get_uid(),
			.hostname = std::string(proc.owning_os->get_hostname()),
			.address = player_address
		});

		proc.putln("Connecting this host to ISP '{}'.", target_isp->hostname);
		outer_world.import_data(builder.data());
		proc.putln("Imported data successfully.");
		proc.putln("Generated network:");
		proc.putln("- Central Routing Node ({})", hub_address);
		print_generated_modules(proc, generated, 1);

	}

	co_return 0;
}
