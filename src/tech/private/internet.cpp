#include "internet.h"
#include "net_node.h"

#include <queue>
#include <ranges>

#include <iso646.h>

void Internet::register_node(Uid64 mac, std::shared_ptr<NetNode> node)
{
	nodes_[mac] = std::move(node);
}

void Internet::unregister_node(Uid64 mac)
{
	unlink_all(mac);
	nodes_.erase(mac);
}

void Internet::link(Uid64 from, Uid64 to)
{
	if (nodes_.contains(from) && nodes_.contains(to))
	{
		links_.relate(from, to);
		nodes_[from]->notify_link(to);
		nodes_[to]->notify_link(from);
	}
}

void Internet::unlink(Uid64 from, Uid64 to)
{
	if (auto it = nodes_.find(from); it != nodes_.end())
	{
		it->second->notify_unlink(to);
	}

	if (auto it = nodes_.find(to); it != nodes_.end())
	{
		it->second->notify_unlink(from);
	}

	links_.unrelate(from, to);
}

void Internet::unlink_all(Uid64 node)
{
	for (auto&& idx : links_.get_all_related(node))
	{
		nodes_[node]->notify_unlink(idx);
	}

	links_.unrelate_all(node);
}

NetPathResult Internet::get_path(Uid64 from, Uid64 to) const
{
	std::queue<Uid64> frontier;
	std::unordered_map<Uid64, Uid64> came_from;
	
	frontier.emplace(from);
	came_from[from] = from;

	while (not frontier.empty())
	{
		auto current = frontier.front();

		if (current == to)
			break;

		for (auto&& next : links_.get_all_related(current))
		{
			if (not came_from.contains(next))
			{
				frontier.emplace(next);
				came_from[next] = current;
			}
			
		}

		frontier.pop();
	}

	/* Reconstruct path. */
	if (came_from.contains(to))
	{
		Uid64 current = to;
		std::vector<Uid64> path;
		while (current != from)
		{
			path.emplace_back(current);
			current = came_from[current];
		}
		path.emplace_back(from);
		std::ranges::reverse(path);
		return std::make_pair(true, std::move(path));
	}

	return std::make_pair(false, std::vector<Uid64>());
}

size_t Internet::distance(Uid64 from, Uid64 to) const
{
	auto path = get_path(from, to);
	return path.first ? path.second.size() : 0;
}

bool Internet::link_unicast(Uid64 from, Uid64 to, ip::IpPackage&& pak)
{
	if (links_.is_related(from, to))
	{
		return link_receive(to, std::move(pak));
	}
	else return false;
}

int32_t Internet::link_broadcast(Uid64 from, ip::IpPackage&& pak)
{
	int32_t rx = 0;
	for (auto&& rec : links_.get_all_related(from))
	{
		if (rec == from)
			continue;

		rx += link_receive(rec, std::forward<ip::IpPackage>(pak));
	}

	return rx;
}

bool Internet::link_receive(Uid64 to, ip::IpPackage&& pak)
{
	if (auto it = nodes_.find(to); it != nodes_.end())
	{
		/* If the node is local, we deliver the message. 
		However, it could also be remote, meaning the message needs to be transmitted
		over a real internet connection. */
		it->second->route.push(std::move(pak));
		return true;
	}
	return false;
}
