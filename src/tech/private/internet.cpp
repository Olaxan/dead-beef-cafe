#include "internet.h"

void Internet::register_node(Uid64 mac, NetNode* node)
{
	nodes_[mac] = node;
}

void Internet::unregister_node(Uid64 mac)
{
	nodes_.erase(mac);
}

void Internet::link(Uid64 from, Uid64 to)
{
	links_.relate(from, to);
}

void Internet::unlink(Uid64 from, Uid64 to)
{
	links_.unrelate(from, to);
}

void Internet::unlink_all(Uid64 node)
{
	links_.unrelate_all(node);
}

int32_t Internet::distance(Uid64 from, Uid64 to)
{
	return 0;
}

bool Internet::link_unicast(Uid64 from, Uid64 to, ip::IpPackage&& pak)
{
	if (links_.is_related(from, to))
	{
		return link_receive(to, std::move(pak));
	}
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
	return false;
}
