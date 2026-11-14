#include "world_builder.h"

#include "host.h"
#include "os.h"
#include "world_data.h"

#include <stdexcept>

namespace WorldGen
{

	/* --- Cluster --- */

	Cluster::Cluster(std::string name, Uid64 root)
		: name_(std::move(name)), root_(root) { }

	void Cluster::add_host(Uid64 uid)
	{
		hosts_.emplace_back(uid);
	}


	/* --- WorldBuilder --- */

	WorldBuilder::WorldBuilder(std::shared_ptr<WorldData> data)
		: world_data_(data) { }
	
	Uid64 WorldBuilder::generate_host(std::string hostname)
	{
		Uid64 uid = Uid64Generator::generate();
		auto host_ptr = std::make_unique<Host>(hostname);

		/* Error check here, for uniqueness. */
		world_data_->hosts.emplace(uid, std::move(host_ptr));

		return uid;
	}

	Uid64 WorldBuilder::generate_cluster(std::string name, Uid64 link_from, ClusterParams params)
	{
		Uid64 hub_uid = generate_host(name);

		gen_data_.clusters_.try_emplace(hub_uid, name, hub_uid);
		Cluster& cluster = gen_data_.clusters_.at(hub_uid);

		world_data_->links.relate(hub_uid, link_from);

		for (std::size_t i = 0; i < params.number_of_hosts; ++i)
		{
			std::string hostname = (params.hostname_generator)
				? params.hostname_generator()
				: std::format("Host-{}", i);

			Uid64 host_uid = generate_host(hostname);
			cluster.add_host(host_uid);

			world_data_->links.relate(hub_uid, host_uid);
		}

		return hub_uid;
	}

	std::vector<GeneratedModule> WorldBuilder::execute(
		const GenerationPlan& plan,
		Uid64 upstream,
		std::mt19937& gen)
	{
		return execute_modules(plan.modules, upstream, gen);
	}

	std::vector<GeneratedModule> WorldBuilder::execute_modules(
		const std::vector<GenerationModule>& modules,
		Uid64 upstream,
		std::mt19937& gen)
	{
		std::vector<GeneratedModule> generated;

		for (const GenerationModule& module : modules)
		{
			if (module.min_instances > module.max_instances || module.min_hosts > module.max_hosts)
				throw std::invalid_argument("Generation module range minimum exceeds maximum.");

			std::uniform_int_distribution<std::size_t> instance_count(module.min_instances, module.max_instances);
			std::uniform_int_distribution<std::size_t> host_count(module.min_hosts, module.max_hosts);

			for (std::size_t i = 0, count = instance_count(gen); i < count; ++i)
			{
				std::string name = module.name_generator
					? module.name_generator()
					: std::format("{}-{}", module.name, i + 1);
				std::size_t hosts = host_count(gen);
				std::vector<std::string> hostnames;
				ClusterParams cluster_params
				{
					.number_of_hosts = hosts,
					.hostname_generator = [&hostnames]()
					{
						std::string hostname = std::format("Host-{}", hostnames.size());
						hostnames.emplace_back(hostname);
						return hostname;
					}
				};

				Uid64 root = generate_cluster(
					name,
					upstream,
					std::move(cluster_params));

				GeneratedModule instance
				{
					.name = module.name,
					.hostname = std::move(name),
					.root = root,
					.hosts = std::move(hostnames)
				};
				instance.children = execute_modules(module.children, root, gen);
				generated.emplace_back(std::move(instance));
			}
		}

		return generated;
	}

	void WorldBuilder::build()
	{
		for (auto&& [uid, cluster] : gen_data_.clusters_)
		{

		}
	}

	Host* WorldBuilder::get_host(Uid64 uid)
	{
		if (auto it = world_data_->hosts.find(uid); it != world_data_->hosts.end())
		{
			return it->second.get();
		}
		return nullptr;
	}
}
