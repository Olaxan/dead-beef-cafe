#pragma once

#include "uid64.h"

#include <memory>
#include <vector>
#include <tuple>
#include <functional>

class Host;
struct WorldData;

namespace WorldGen
{

	using NameGenerator = std::function<std::string(void)>;

	struct ClusterParams
	{
		std::size_t number_of_hosts{1};
		NameGenerator hostname_generator{nullptr};
	};

	class Cluster
	{
	public:

		Cluster(std::string name);

		void add_host(Uid64 uid);
	
	protected:

		std::string name_;
		std::vector<Uid64> hosts_;
		
	};
	
	class WorldBuilder
	{
	public:
	
		WorldBuilder(std::shared_ptr<WorldData> data);

		Uid64 generate_host(std::string hostname);
		Cluster& generate_cluster(std::string name, Uid64 link_from, ClusterParams params);

		void build();
		std::shared_ptr<WorldData> data() { return world_data_; }
	
	protected:
	
		std::shared_ptr<WorldData> world_data_{};

		struct GenerationData
		{
			std::vector<Cluster> clusters_{};
		} gen_data_;
	};
};
