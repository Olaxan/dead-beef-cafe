#include "world_builder.h"

#include "host.h"
#include "os.h"
#include "world_data.h"

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

	Cluster& WorldBuilder::generate_cluster(std::string name, Uid64 link_from, ClusterParams params)
	{
		Uid64 hub_uid = generate_host(name);

		Cluster& cluster = gen_data_.clusters_.emplace_back(name, hub_uid);

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
		
		return cluster;
	}

	void WorldBuilder::build()
	{
	}
}
