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

	std::optional<std::string> read_rx();
	std::optional<std::string> read_tx();

	MessageQueue<ip::IpPackage> rx_queue{};
	MessageQueue<ip::IpPackage> tx_queue{};

	AddressPair local_endpoint{};
	AddressPair remote_endpoint{};

};