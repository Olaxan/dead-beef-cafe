#pragma once

#include "uid64.h"
#include "addr.h"

#include <memory>
#include <vector>
#include <tuple>
#include <functional>
#include <unordered_map>

class Host;
struct WorldData;

namespace WorldGen
{

	using NameGenerator = std::function<std::string(void)>;

	struct ClusterParams
	{
		std::size_t number_of_hosts{1};
		NameGenerator hostname_generator{nullptr};

		Address6 subnet;
		int32_t prefix{0};
	};

	class Cluster
	{
	public:

		Cluster(std::string name, Uid64 root);

		void add_host(Uid64 uid);

		Uid64 get_root() const { return root_; }
	
	protected:

		std::string name_;
		Uid64 root_{0};
		std::vector<Uid64> hosts_;
		
	};
	
	class WorldBuilder
	{
	public:
	
		WorldBuilder(std::shared_ptr<WorldData> data);

		Uid64 generate_host(std::string hostname);
		Uid64 generate_cluster(std::string name, Uid64 link_from, ClusterParams params);

		void build();
		std::shared_ptr<WorldData> data() { return world_data_; }

	protected:

		Host* get_host(Uid64 uid);
	
	protected:
	
		std::shared_ptr<WorldData> world_data_{};

		struct GenerationData
		{
			std::unordered_map<Uid64, Cluster> clusters_{};
		} gen_data_;
	};
};
