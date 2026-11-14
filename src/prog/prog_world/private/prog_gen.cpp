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

#include <algorithm>
#include <string>
#include <cstdlib>
#include <print>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>
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

	world::GenModule& isp_module = *out_params.add_modules();
	isp_module.set_module_name("ISP");
	isp_module.set_min_instances(4);
	isp_module.set_max_instances(8);
	isp_module.set_network_prefix(16);

	world::GenModule& household_module = *isp_module.add_children();
	household_module.set_module_name("Household");
	household_module.set_min_instances(2);
	household_module.set_max_instances(5);
	household_module.set_min_hosts(1);
	household_module.set_max_hosts(6);
	household_module.set_network_prefix(64);

	world::GenModule& corporation_module = *isp_module.add_children();
	corporation_module.set_module_name("Corporation");
	corporation_module.set_min_instances(1);
	corporation_module.set_max_instances(3);
	corporation_module.set_min_hosts(3);
	corporation_module.set_max_hosts(10);
	corporation_module.set_network_prefix(32);

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

		World& outer_world = *proc.owning_os->get_outer_as<World*>();

		auto gen_data = std::make_shared<WorldData>();
		WorldBuilder builder{gen_data};

		std::size_t shared_index{0};

		std::random_device rd;
		std::mt19937 gen(rd());

		auto pick_from_string_field = [&](std::string fallback, const google::protobuf::RepeatedPtrField<std::string>& field) -> std::string
		{
			if (field.empty())
				return std::format("{}-{}", fallback, shared_index++);

			std::uniform_int_distribution<> dist(0, field.size() - 1);
			return field.at(dist(gen));
		};

		auto generate_name_from_pool = [&](const world::NamePool& pool, const std::string& fallback) -> std::string
		{
			std::string prefix = pick_from_string_field(fallback, pool.prefix());
			if (pool.suffix().empty())
				return prefix;

			return std::format("{} {}", prefix, pick_from_string_field(fallback, pool.suffix()));
		};

		auto has_names = [](const world::NamePool& pool)
		{
			return not pool.prefix().empty() or not pool.suffix().empty();
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
			if (module.module_name().empty())
			{
				proc.errln("Generation modules must have a moduleName.");
				return false;
			}
			const uint32_t min_instances = module.has_min_instances() ? module.min_instances() : 1;
			const uint32_t max_instances = module.has_max_instances() ? module.max_instances() : min_instances;
			const uint32_t min_hosts = module.has_min_hosts() ? module.min_hosts() : 0;
			const uint32_t max_hosts = module.has_max_hosts() ? module.max_hosts() : min_hosts;
			const uint32_t network_prefix = module.has_network_prefix()
				? module.network_prefix()
				: default_network_prefix(module.module_name(), parent_prefix);
			if (min_instances > max_instances)
			{
				proc.errln("Generation module '{}' has minInstances greater than maxInstances.", module.module_name());
				return false;
			}
			if (min_hosts > max_hosts)
			{
				proc.errln("Generation module '{}' has minHosts greater than maxHosts.", module.module_name());
				return false;
			}
			if (network_prefix <= parent_prefix || network_prefix > 127)
			{
				proc.errln("Generation module '{}' must have a networkPrefix between {} and 127.", module.module_name(), parent_prefix + 1);
				return false;
			}
			for (const world::GenModule& child : module.children())
			{
				if (not module_is_valid(child, static_cast<uint8_t>(network_prefix)))
					return false;
			}
			return true;
		};

		for (const world::GenModule& module : gen_params.modules())
		{
			if (not module_is_valid(module, 8))
				co_return 1;
		}

		GenerationPlan plan;
		std::function<GenerationModule(const world::GenModule&, const world::GenCountry&, uint8_t)> convert_module;
		convert_module = [&](const world::GenModule& source, const world::GenCountry& country, uint8_t parent_prefix)
		{
			GenerationModule module;
			module.name = source.module_name();
			module.min_instances = source.has_min_instances() ? source.min_instances() : 1;
			module.max_instances = source.has_max_instances() ? source.max_instances() : module.min_instances;
			module.min_hosts = source.has_min_hosts() ? source.min_hosts() : 0;
			module.max_hosts = source.has_max_hosts() ? source.max_hosts() : module.min_hosts;
			module.network_prefix = static_cast<uint8_t>(source.has_network_prefix()
				? source.network_prefix()
				: default_network_prefix(module.name, parent_prefix));

			const world::NamePool* name_pool = nullptr;
			if (source.has_name_pool())
				name_pool = &source.name_pool();
			else if (module.name == "ISP" and country.has_isp_names())
				name_pool = &country.isp_names();
			else if (module.name == "Corporation" and country.has_business_names())
				name_pool = &country.business_names();
			else if (module.name == "Household" and country.has_person_names())
				name_pool = &country.person_names();

			if (name_pool and has_names(*name_pool))
			{
				module.name_generator = [&, name_pool, fallback = module.name]()
				{
					return generate_name_from_pool(*name_pool, fallback);
				};
			}

			for (const world::GenModule& child : source.children())
				module.children.emplace_back(convert_module(child, country, module.network_prefix));

			return module;
		};

		for (const world::GenCountry& country : gen_params.countries())
		{
			GenerationModule country_module;
			country_module.name = "Country";
			country_module.network_prefix = 8;
			const world::GenCountry* country_ptr = &country;
			country_module.name_generator = [country_ptr]()
			{
				return country_ptr->country_name();
			};
			for (const world::GenModule& module : gen_params.modules())
				country_module.children.emplace_back(convert_module(module, country, country_module.network_prefix));

			if (gen_params.modules().empty())
			{
				world::GenModule fallback;
				fallback.set_module_name("ISP");
				fallback.set_min_instances(4);
				fallback.set_max_instances(8);
				fallback.set_min_hosts(4);
				fallback.set_max_hosts(8);
				fallback.set_network_prefix(16);
				country_module.children.emplace_back(convert_module(fallback, country, country_module.network_prefix));
			}
			plan.modules.emplace_back(std::move(country_module));
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

		builder.build();

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
		gen_data->links.relate(target_isp->root, proc.owning_os->get_uid());
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
