#include "world_builder.h"

#include "host.h"
#include "os.h"
#include "world_data.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace WorldGen
{
	namespace
	{
		Address6 mask_subnet(Address6 address, uint8_t prefix)
		{
			if (prefix > 128)
				throw std::invalid_argument("IPv6 network prefix exceeds 128 bits.");

			for (std::size_t bit = prefix; bit < 128; ++bit)
			{
				address.bytes[bit / 8] &= static_cast<uint8_t>(~(1u << (7 - bit % 8)));
			}
			return address;
		}

		Address6 allocate_subnet(
			Address6 address_space,
			uint8_t address_space_prefix,
			uint8_t allocation_prefix,
			std::size_t index)
		{
			if (address_space_prefix >= allocation_prefix || allocation_prefix > 128)
				throw std::invalid_argument("Child network prefix must be longer than its address space prefix.");

			const std::size_t index_bits = allocation_prefix - address_space_prefix;
			if (index_bits < std::numeric_limits<std::size_t>::digits &&
				index >= (std::size_t{1} << index_bits))
			{
				throw std::length_error("Generation plan exceeds the available IPv6 subnets.");
			}

			Address6 subnet = mask_subnet(address_space, allocation_prefix);
			for (std::size_t bit = 0; bit < index_bits; ++bit)
			{
				const std::size_t index_bit = index_bits - bit - 1;
				if (index_bit < std::numeric_limits<std::size_t>::digits &&
					((index >> index_bit) & 1u) != 0)
				{
					subnet.bytes[(address_space_prefix + bit) / 8] |=
						static_cast<uint8_t>(1u << (7 - (address_space_prefix + bit) % 8));
				}
			}
			return subnet;
		}

		Address6 add_address_offset(Address6 address, std::size_t offset)
		{
			std::size_t carry = offset;
			for (std::size_t i = address.bytes.size(); i > 0 && carry > 0; --i)
			{
				const std::size_t sum = address.bytes[i - 1] + (carry & 0xFFu);
				address.bytes[i - 1] = static_cast<uint8_t>(sum & 0xFFu);
				carry = (carry >> 8) + (sum >> 8);
			}

			if (carry != 0)
				throw std::overflow_error("Generated IPv6 host address overflowed.");

			return address;
		}

		void validate_host_capacity(uint8_t prefix, std::size_t number_of_hosts)
		{
			const std::size_t host_bits = 128 - prefix;
			if (host_bits < std::numeric_limits<std::size_t>::digits)
			{
				const std::size_t subnet_size = std::size_t{1} << host_bits;
				if (number_of_hosts >= subnet_size - 1)
					throw std::length_error("Generation module has more hosts than its subnet can contain.");
			}
		}
	}

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

		world_data_->links->relate(link_from, hub_uid);

		for (std::size_t i = 0; i < params.number_of_hosts; ++i)
		{
			std::string hostname = (params.hostname_generator)
				? params.hostname_generator()
				: std::format("Host-{}", i);

			Uid64 host_uid = generate_host(hostname);
			cluster.add_host(host_uid);

			world_data_->links->relate(hub_uid, host_uid);
		}

		return hub_uid;
	}

	std::vector<GeneratedModule> WorldBuilder::execute(
		const GenerationPlan& plan,
		Uid64 upstream,
		std::mt19937& gen,
		Address6 address_space,
		uint8_t address_space_prefix)
	{
		return execute_modules(plan.modules, upstream, address_space, address_space_prefix, gen);
	}

	std::vector<GeneratedModule> WorldBuilder::execute_modules(
		const std::vector<GenerationModule>& modules,
		Uid64 upstream,
		Address6 address_space,
		uint8_t address_space_prefix,
		std::mt19937& gen)
	{
		std::vector<GeneratedModule> generated;
		if (modules.empty())
			return generated;

		uint8_t allocation_prefix = 128;
		std::size_t maximum_instances = 0;
		for (const GenerationModule& module : modules)
		{
			if (module.min_instances > module.max_instances || module.min_hosts > module.max_hosts)
				throw std::invalid_argument("Generation module range minimum exceeds maximum.");
			if (module.network_prefix <= address_space_prefix || module.network_prefix > 127)
				throw std::invalid_argument("Generation module network prefix must be within its parent network.");
			validate_host_capacity(module.network_prefix, module.max_hosts);

			allocation_prefix = std::min(allocation_prefix, module.network_prefix);
		}

		const std::size_t index_bits = allocation_prefix - address_space_prefix;
		const std::size_t available_subnets =
			index_bits < std::numeric_limits<std::size_t>::digits
				? (std::size_t{1} << index_bits) - 1
				: std::numeric_limits<std::size_t>::max();
		for (const GenerationModule& module : modules)
		{
			if (module.max_instances > available_subnets - maximum_instances)
				throw std::length_error("Generation plan exceeds the available IPv6 subnets.");
			maximum_instances += module.max_instances;
		}

		std::size_t subnet_index = 1;

		for (const GenerationModule& module : modules)
		{
			std::uniform_int_distribution<std::size_t> instance_count(module.min_instances, module.max_instances);
			std::uniform_int_distribution<std::size_t> host_count(module.min_hosts, module.max_hosts);

			for (std::size_t i = 0, count = instance_count(gen); i < count; ++i)
			{
				Address6 allocation = allocate_subnet(
					address_space,
					address_space_prefix,
					allocation_prefix,
					subnet_index++);
				Address6 subnet = mask_subnet(allocation, module.network_prefix);
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
					},
					.subnet = subnet,
					.prefix = module.network_prefix
				};

				Uid64 root = generate_cluster(
					name,
					upstream,
					std::move(cluster_params));

				const std::vector<Uid64>& host_uids = gen_data_.clusters_.at(root).get_hosts();
				GeneratedModule instance
				{
					.name = module.name,
					.hostname = std::move(name),
					.root = root,
					.subnet = subnet,
					.address = add_address_offset(subnet, 1),
					.next_host_address = add_address_offset(subnet, host_uids.size() + 2),
					.network_prefix = module.network_prefix
				};
				for (std::size_t host_index = 0; host_index < host_uids.size(); ++host_index)
				{
					instance.hosts.emplace_back(GeneratedHost{
						.uid = host_uids[host_index],
						.hostname = std::move(hostnames[host_index]),
						.address = add_address_offset(subnet, host_index + 2)
					});
				}
				instance.children = execute_modules(
					module.children,
					root,
					subnet,
					module.network_prefix,
					gen);
				generated.emplace_back(std::move(instance));
			}
		}

		return generated;
	}
}
