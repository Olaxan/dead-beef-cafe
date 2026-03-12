#pragma once

#include "file.h"
#include "msg_queue.h"
#include "addr.h"

#include "proto/ip_packet.pb.h"

class SocketFile : public File
{
public:

	SocketFile() = default;
	SocketFile(uint64_t fid) : File(fid) { };
	~SocketFile() = default;

};