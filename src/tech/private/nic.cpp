#include "nic.h"

#include "host.h"
#include "link_srv.h"
#include "msg_queue.h"
#include "uid64.h"

#include <print>
#include <algorithm>

NIC::~NIC() = default;

void NIC::on_start(Host* owner)
{
	//LinkServer& internet = owner->get_world().get_link_server();
	//internet.register_node(this);
}

void NIC::on_shutdown(Host* owner)
{
	//LinkServer& internet = owner->get_world().get_link_server();
	//internet.unregister_node(this);
}