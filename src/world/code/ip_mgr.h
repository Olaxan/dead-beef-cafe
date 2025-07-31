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


	/* Sockets management */
	ISocket* resolve(Address6 addr, int32_t port)
	{
		auto it = sockets_.find({addr, port});
		return (it != sockets_.end()) ? it->second.get() : nullptr;
	}

	bool bind(std::shared_ptr<ISocket> sock, Address6 listen_ip, int32_t listen_port)
	{
		AddressPair addr = {listen_ip, listen_port};

		if (sockets_.contains(addr))
			return false;

		std::println("Binding socket {0} to [{1}]:{2}.", (int64_t)sock.get(), listen_ip, listen_port);

		sockets_[addr] = std::move(sock);

		return true;
	}

	template<typename TargetT>
	std::optional<SessionId> connect(std::shared_ptr<ISocket> sock, Address6 dest_addr, int32_t dest_port)
	{
		ISocket* remote = resolve(dest_addr, dest_port);

		if (remote == nullptr)
			return std::nullopt;

		std::println("Connecting socket {0} to {1}:{2}.", (int64_t)sock.get(), dest_addr, dest_port);

		if (SocketStreamFn stream = sock->open_stream(remote); stream != nullptr)
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