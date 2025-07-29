#pragma once

#include "netw.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <concepts>
#include <optional>
#include <memory>

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


	/* Sockets management */
	ISocket* resolve(Address6 addr, int32_t port)
	{
		auto it = sockets_.find({addr, port});
		return (it != sockets_.end()) ? it->second.get() : nullptr;
	}

	bool bind(std::shared_ptr<ISocket> sock, Address6 dest_ip, int32_t dest_port)
	{
		AddressPair addr = {dest_ip, dest_port};

		if (sockets_.contains(addr))
			return false;

		sockets_[addr] = std::move(sock);

		return true;
	}

	template<typename TargetT>
	std::optional<SessionId> connect(std::shared_ptr<ISocket> in, Address6 addr, int32_t port)
	{
		ISocket* out = resolve(addr, port);

		if (out == nullptr)
			return std::nullopt;

		if (SocketStreamFn stream = in->open_stream(out); stream != nullptr)
		{
			streams_.push_back(std::move(stream));
			return SessionId{0};
		}

		return std::nullopt;
	}

private:

	std::unordered_map<std::string, IEndpoint*> endpoints_{};
	std::unordered_map<AddressPair, std::shared_ptr<ISocket>> sockets_{};
	std::vector<SocketStreamFn> streams_{};
	
};