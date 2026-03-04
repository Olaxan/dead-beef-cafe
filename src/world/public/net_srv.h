#pragma once

#include "netw.h"
#include "addr.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <concepts>
#include <optional>
#include <memory>
#include <print>

class IEndpoint;

struct SessionId
{
	int32_t id{0};
};

class IpManager
{
public:

	IpManager() = default;

	void step(float delta_seconds);

	/* NIC/endpoint management */
	void register_endpoint(IEndpoint* endpoint, std::string ip)
	{
		endpoints_[ip] = endpoint;
	}

	bool unregister_endpoint(IEndpoint* endpoint, std::string ip)
	{
		return endpoints_.erase(ip) > 0;
	}
	/* End NIC/endpoint management*/

private:

	std::unordered_map<std::string, IEndpoint*> endpoints_{};
	
};