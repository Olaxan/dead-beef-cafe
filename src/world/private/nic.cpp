#include "nic.h"

#include "world.h"
#include "host.h"
#include "ip_mgr.h"

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
