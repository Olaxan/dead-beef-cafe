#pragma once

#include "msg_queue.h"
#include "net_types.h"
#include "uid64.h"

#include <proto/ip_packet.pb.h>

struct NetNode
{
	NetQueue route;

	void notify_link(Uid64 other) {};
	void notify_unlink(Uid64 other) {};
};