#pragma once

#include "msg_queue.h"
#include "addr.h"
#include "uuid.h"

#include "proto/ip_packet.pb.h"
#include "proto/tcp_packet.pb.h"

#include <cstdint>
#include <memory>
#include <coroutine>
#include <functional>
#include <print>
#include <tuple>

class NIC;
class NetManager;
class SocketFile;

using NetQueue = MessageQueue<ip::IpPackage>;
using NetMessageAwaiter = MessageQueueAwaiter<ip::IpPackage>;

using OpenSocketHandle = int64_t;

struct OpenSocketEntry
{
	MessageQueue<ip::IpPackage> rx_queue{};
	MessageQueue<ip::IpPackage> tx_queue{};

	AddressPair local_endpoint{};
	AddressPair remote_endpoint{};

	bool open{false};
};

using OpenSocketPair = std::pair<OpenSocketHandle, OpenSocketEntry*>;

using NetCastFn = std::function<void(Uid64, NIC*)>;

using NetReadResult = std::expected<std::string, std::error_condition>;
using NetReadResultIp = std::expected<ip::IpPackage, std::error_condition>;
using NetReadResultTcp = std::expected<ip::TcpPacket, std::error_condition>;