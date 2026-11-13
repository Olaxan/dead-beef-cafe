#pragma once

#include "uid64.h"

#include <memory>
#include <vector>
#include <tuple>
#include <functional>

class Host;
class WorldData;

namespace WorldGen
{

	template<typename T>
	using NameGenerator<T> = std::function<std::string(const T&)>;

	struct ClusterParams
	{
		std::size_t number_of_hosts{1};
		NameGenerator<Host> hostname_generator{nullptr};
	};

	class Cluster
	{
	public:

		Cluster(std::string name);

		Host& add_host(std::string name);
	
	protected:

		std::string name_;
		std::vector<Uid64> hosts_;
		
	};
	
	class WorldBuilder
	{
	public:
	
		WorldBuilder(std::shared_ptr<WorldData> data);

		std::pair<Uid64, Host*> add_host(std::string hostname);
		Cluster& generate_cluster(std::string name, Uid64 link_from, ClusterParams params);
	
	
	
	protected:
	
		std::shared_ptr<WorldData> data_{};
	};
};
