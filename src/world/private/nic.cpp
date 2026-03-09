#include "nic.h"

#include "world.h"
#include "host.h"
#include "link_srv.h"
#include "msg_queue.h"
#include "uuid.h"

#include <print>

NIC::~NIC() = default;

void NIC::set_ip(const std::string& new_ip)
{
	address_ = Address6::from_string(new_ip).value_or(Address6{});
}

void NIC::on_linked(LinkServer* links, ILinkable* other)
{
	std::println("Linked {} to {}.", UUID{reinterpret_cast<uint64_t>(this)}, UUID{reinterpret_cast<uint64_t>(other)});
}

void NIC::on_unlinked(LinkServer* links)
{
	std::println("Unlinked NIC.");
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

void NIC::transfer_one(NIC* other)
{
	
}
