#pragma once

#include "msg_queue.h"
#include "net_types.h"
#include "uid64.h"

struct NetNode
{
	FrameQueue rx_buffer;
	FrameQueue tx_buffer;

	void notify_link(Uid64 other) {};
	void notify_unlink(Uid64 other) {};
};