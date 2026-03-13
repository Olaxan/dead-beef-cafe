#include "nic.h"

#include "world.h"
#include "host.h"
#include "link_srv.h"
#include "msg_queue.h"
#include "uuid.h"

#include <print>
#include <algorithm>

NIC::~NIC() = default;

void NIC::set_ip(const std::string& new_ip)
{
	address_ = Address6::from_string(new_ip).value_or(Address6{0});
}

void NIC::on_linked(LinkServer* links, ILinkable* other)
{
	assert(other);
	//std::println("Linked {} to {}.", Uid64{reinterpret_cast<uint64_t>(this)}, Uid64{reinterpret_cast<uint64_t>(other)});

	NIC* other_nic{static_cast<NIC*>(other)};
	Uid64 other_mac{reinterpret_cast<uint64_t>(other_nic)};

	link_cache_[other_mac] = other_nic;
	notify_link_update(other_mac, LinkUpdateType::LinkAdded);
}

void NIC::on_unlinked(LinkServer* links, ILinkable* other)
{
	assert(other);
	//std::println("Unlinked {} from {}.", Uid64{reinterpret_cast<uint64_t>(this)}, Uid64{reinterpret_cast<uint64_t>(other)});

	NIC* other_nic{static_cast<NIC*>(other)};
	Uid64 other_mac{reinterpret_cast<uint64_t>(other_nic)};

	link_cache_.erase(other_mac);
	notify_link_update(other_mac, LinkUpdateType::LinkRemoved);
}

void NIC::on_start(Host* owner)
{
	LinkServer& internet = owner->get_world().get_link_server();
	internet.register_node(this);
}

void NIC::on_shutdown(Host* owner)
{
	LinkServer& internet = owner->get_world().get_link_server();
	internet.unregister_node(this);
}

size_t NIC::transfer(Uid64 mac, ip::IpPackage&& packet)
{
	if (auto it = link_cache_.find(mac); it != link_cache_.end())
	{
		size_t bytes = packet.ByteSizeLong();
		NIC* other = it->second;
		other->rx_queue_.push(std::move(packet));
		return bytes;
	}

	return 0;
}

void NIC::broadcast(NetCastFn broadcast_fn)
{
	for (auto& [uuid, nic] : link_cache_)
		broadcast_fn(uuid, nic);
}

void NIC::unicast(Uid64 mac, NetCastFn unicast_fn)
{
	if (auto it = link_cache_.find(mac); it != link_cache_.end())
		unicast_fn(mac, it->second);
}

void NIC::add_link_update_callback(LinkUpdateCallbackFn&& fn)
{
	callbacks_.push_back(std::move(fn));
}

void NIC::notify_link_update(Uid64 mac, LinkUpdateType type)
{
	for (auto&& fn : callbacks_)
	{
		fn(std::make_pair(mac, type));
	}
}