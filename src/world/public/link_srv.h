#pragma once

#include "soa_helpers.h"
#include "msg_queue.h"
#include "task.h"

#include <set>
#include <coroutine>
#include <vector>
#include <unordered_map>

using NetNotifyFn = std::function<void(void)>;

template<typename T>
using NetServerContainerBase = std::vector<T, std::allocator<T>>;

enum NetServerComponents : uint8_t
{
	ComNetRxQueue,
    ComNetTxQueue,
};

template<typename ... T>
struct NetServerData : public std::tuple<T ...>
{
	using std::tuple<T...>::tuple;
 	auto& get_rx_queue() { return std::get<ComNetRxQueue>(*this); }
	auto& get_tx_queue() { return std::get<ComNetTxQueue>(*this); }
};

using NetServerDataTypes = NetServerData<
	MessageQueue<std::string>,	// RX
	MessageQueue<std::string>>;	// TX

using NetServerContainer = BaseContainer<NetServerContainerBase, DataLayout::SoA, NetServerDataTypes>;

template <typename T>
class NetServer
{
public:

	NetServer() = default;

	/* Adds a link between two nodes. */
	void link(T first, T second)
	{
		links_.insert(std::pair{first, second});
		links_.insert(std::pair{second, first});
	}

	/* Removes a specific link between two nodes. */
	void unlink(T first, T second)
	{
		links_.erase(std::pair{first, second});
		links_.erase(std::pair{second, first});
	}

	/* Removes a node entirely from the directory,
	and removes all links to and from it. */
	void purge(T node)
	{
		links_.erase(node);
		auto range = links_.equal_range(node);
		for (auto it = range.first; it != range.second;)
		{
			if (it->second == node)
				it = links_.erase(it);
			else
				++it;
		}
	}

protected:

	NetServerContainer data_{};
	std::unordered_multimap<T, T> links_{};

};