#include "nic.h"

#include "world.h"
#include "host.h"
#include "link_srv.h"
#include "msg_queue.h"

NIC::~NIC() = default;

void NIC::set_ip(const std::string& new_ip)
{
	address_ = Address6::from_string(new_ip).value_or(Address6{});
}

void NIC::on_start(Host* owner)
{
	IpLinkServer& internet = owner->get_world().get_link_server();
	internet.register_node(this);
}

void NIC::on_shutdown(Host* owner)
{
	IpLinkServer& internet = owner->get_world().get_link_server();
	internet.unregister_node(this);
}

void NIC::transfer_one(NIC* other)
{
	// if (std::optional<ip::IpPackage> pop = tx_queue_.pop())
	// {
	// 	other->rx_queue_.push(*pop);
	// }
}
