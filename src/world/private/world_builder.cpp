#include "world_builder.h"

#include "host.h"
#include "os.h"
#include "world_data.h"

namespace WorldGen
{

	Cluster::Cluster(std::string name)
		: name_(std::move(name))
	{
	}

	WorldBuilder::WorldBuilder(std::shared_ptr<WorldData> data)
		: world_data_(data) { }
	
	Uid64 WorldBuilder::add_host(std::string hostname)
	{
		// Uid64 uid = Uid64Generator::generate();
		// auto host_ptr = std::make_unique<Host>(hostname, uid);

		// if (auto pair = data_->hosts.emplace(uid, std::move(host_ptr)); pair.second)
		// {
		// 	return std::make_pair(uid, pair.first)
		// }
		// else return std::make_pair(uid{0}, nullptr);
		return Uid64{0};
	}

	Cluster& WorldBuilder::generate_cluster(std::string name, Uid64 link_from, ClusterParams params)
	{
		Cluster& cluster = gen_data_.clusters_.emplace_back(name);

		std::string hub_hostname = params.hostname_generator();
		Uid64 hub_uid = add_host(hub_hostname);
		world_data_->links.relate(hub_uid, link_from);

		for (std::size_t i = 0; i < params.number_of_hosts; ++i)
		{
			std::string hostname = params.hostname_generator();
			//Host& host = add_host(hostname);
			
			//data_->links.relate(link_from, uid);
		}
		return cluster;
	}
}
