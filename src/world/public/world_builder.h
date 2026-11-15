#pragma once

#include "uid64.h"
#include "addr.h"

#include <memory>
#include <vector>
#include <tuple>
#include <functional>
#include <unordered_map>
#include <random>
#include <string>

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

	struct GenerationModule
	{
		std::string name;
		std::size_t min_instances{1};
		std::size_t max_instances{1};
		std::size_t min_hosts{0};
		std::size_t max_hosts{0};
		uint8_t network_prefix{64};
		NameGenerator name_generator{nullptr};
		std::vector<GenerationModule> children;
	};

	struct GenerationPlan
	{
		std::vector<GenerationModule> modules;
	};

	struct GeneratedHost
	{
		Uid64 uid{0};
		std::string hostname;
		Address6 address;
	};

	struct GeneratedModule
	{
		std::string name;
		std::string hostname;
		Uid64 root{0};
		Address6 subnet;
		Address6 address;
		Address6 next_host_address;
		uint8_t network_prefix{64};
		std::vector<GeneratedHost> hosts;
		std::vector<GeneratedModule> children;
	};

	class Cluster
	{
	public:

		Cluster(std::string name, Uid64 root);

		void add_host(Uid64 uid);

		Uid64 get_root() const { return root_; }
		const std::vector<Uid64>& get_hosts() const { return hosts_; }
	
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
		std::vector<GeneratedModule> execute(
			const GenerationPlan& plan,
			Uid64 upstream,
			std::mt19937& gen,
			Address6 address_space = {},
			uint8_t address_space_prefix = 0);

		std::shared_ptr<WorldData> data() { return world_data_; }

	protected:

		std::vector<GeneratedModule> execute_modules(
			const std::vector<GenerationModule>& modules,
			Uid64 upstream,
			Address6 address_space,
			uint8_t address_space_prefix,
			std::mt19937& gen);
	
	protected:
	
		std::shared_ptr<WorldData> world_data_{};

		struct GenerationData
		{
			std::unordered_map<Uid64, Cluster> clusters_{};
		} gen_data_;
	};
};
