#pragma once

#include "msg_queue.h"
#include "addr.h"

#include "proto/ip_packet.pb.h"

#include <cstdint>
#include <memory>
#include <coroutine>
#include <print>

using NetQueue = MessageQueue<ip::IpPackage>;
using NetMessageAwaiter = MessageQueueAwaiter<ip::IpPackage>;
