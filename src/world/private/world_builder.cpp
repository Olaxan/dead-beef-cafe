#include "world_builder.h"

#include "host.h"
#include "os.h"
#include "world_data.h"

namespace WorldGen
{
	WorldBuilder::WorldBuilder(std::shared_ptr<WorldData> data)
		: data_(data) { }
	
	std::pair<Uid64, Host*> WorldBuilder::add_host(std::string hostname)
	{
		Uid64 uid = Uid64Generator::generate();
		auto host_ptr = std::make_unique<Host>(hostname, uid);

		if (auto pair = data_->hosts.emplace(uid, std::move(host_ptr)); pair.second)
		{
			return std::make_pair(uid, pair.first)
		}
		else return std::make_pair(uid{0}, nullptr);
	}

	Cluster& WorldBuilder::generate_cluster(std::string name, Uid64 link_from, ClusterParams params)
	{
		OS& os = link_from.get_os();
		NetManager& net_mgr = *os.get_network_manager();

		std::string hub_hostname = params.generate_hostname();
		auto [hub_uid, hub_node] = add_host(hub_hostname);
		data_->links.relate(hub_uid, link_from);

		for (std::size_t i = 0; i < params.number_of_hosts; ++i)
		{
			std::string hostname = params.hostname_generator(*host_ptr);
			Host& host = add_host(hostname);
			
			data_->links.relate(link_from, uid);
		}
	}
}
