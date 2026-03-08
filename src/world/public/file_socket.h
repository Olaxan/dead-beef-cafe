#pragma once

#include "file.h"
#include "msg_queue.h"
#include "addr.h"


class SocketFile : public File
{
public:

	SocketFile() = default;

	SocketFile(uint64_t fid) : File(fid) { };

	~SocketFile() = default;

	void write(std::string content) override;
	std::optional<std::string> read() const override;
	std::optional<std::string> eat() override;

	MessageQueue<std::string> rx_queue{};
	MessageQueue<std::string> tx_queue{};

	AddressPair local_endpoint{};
	AddressPair other_endpoint{};

};