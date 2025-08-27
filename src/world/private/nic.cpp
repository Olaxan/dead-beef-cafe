#include "nic.h"

#include "world.h"
#include "host.h"
#include "net_srv.h"

void NIC::set_ip(const std::string& new_ip)
{
	address_ = Address6::from_string(new_ip).value_or(Address6{});
}

void NIC::on_start(Host* owner)
{
	//IpManager& internet = owner->get_world().get_ip_manager();
	//internet.register_endpoint(this, ip_);
}

void NIC::on_shutdown(Host* owner)
{
	//IpManager& internet = owner->get_world().get_ip_manager();
	//internet.unregister_endpoint(this, ip_);
}
