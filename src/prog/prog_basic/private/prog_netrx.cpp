#include "prog_basic.h"

#include "os.h"
#include "net_types.h"
#include "link_awaiter.h"
#include "filesystem.h"
#include "net_mgr.h"
#include "scoped_fd.h"

#include "proto/ip_packet.pb.h"
#include "proto/arp_packet.pb.h"
#include "proto/dataframe.pb.h"

#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <ranges>
#include <functional>
#include <tuple>

#include <iso646.h>

ProcessTask Programs::SrvNetRx(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	NetManager* net = os.get_network_manager();

	FileAccessFlags flags = FileAccessFlags::Create | FileAccessFlags::Write | FileAccessFlags::Append;
	FileScope log{proc, "/var/log/rx.log", flags};
	proc.set_writer([log = std::move(log)](Proc& wproc, WriteInput str)
	{
		if (str) { std::ignore = log.write(*str); }
	});

	proc.putln("RX service running.");

	auto make_frame = [&](Uid64 dest, ip::FrameType type) -> ip::DataFrame
	{
		ip::DataFrame frame;
		frame.set_src_mac(net->get_mac().num);
		frame.set_dest_mac(dest.num);
		frame.set_frame_type(type);
		return frame;
	};

	auto receive_arp = [&](ip::ArpPackage&& packet, Uid64 frame_src_mac)
	{
		switch (packet.type())
		{
			case ip::ArpType::ANNOUNCE:
			{
				std::println("[{}]: received ARP announce.", net->get_primary_ip());
				auto exp_announce_ip = Address6::from_bytes(packet.src_ip());
				Uid64 announce_mac = packet.src_mac();

				std::string receive = packet.src_ip();

				if (exp_announce_ip)
				{
					Address6 announce_ip = *exp_announce_ip;
					net->arp_refresh(announce_mac, announce_ip);
					proc.putln("Refreshing ARP entry '{}' -> '{}'.", announce_ip, announce_mac);
					return;
				}
				else proc.warnln("Invalid ARP announce: {}", exp_announce_ip.error().what());

				return;
			}
			case ip::ArpType::PROBE:
			{
				std::println("[{}]: received ARP probe.", net->get_primary_ip());
				if (packet.dest_ip() == net->get_primary_ip().data_str())
				{
					proc.putln("Received matched ARP probe.");

					ip::ArpPackage reply;
					reply.set_type(ip::ArpType::ANNOUNCE);
					reply.set_src_ip(net->get_primary_ip().data_str());
					reply.set_src_mac(net->get_mac().num);

					std::string send = net->get_primary_ip().data_str();

					auto frame = make_frame(frame_src_mac, ip::FrameType::ARP);
					frame.set_payload(reply.SerializeAsString());
					net->send(std::move(frame));
				}

				return;
			}
		}
	};

	auto receive_ip = [&](ip::IpPackage&& packet, Uid64 src_mac)
	{
		auto exp_src_addr = Address6::from_bytes(packet.src_ip());
		auto exp_dest_addr = Address6::from_bytes(packet.dest_ip());

		std::println("[{}]: received IP.", net->get_primary_ip());

		if (exp_src_addr && exp_dest_addr)
		{
			const Address6& src_ip = *exp_src_addr;
			const Address6& dest_ip = *exp_dest_addr;

			std::println("[{}]: from={} to={}.", net->get_primary_ip(), src_ip, dest_ip);

			//net->arp_refresh(src_mac, src_ip);

			if (dest_ip == net->get_primary_ip())
			{
				net->unpack(std::move(packet), src_ip, dest_ip);
			}
			else
			{
				net->send(std::move(packet));
			}
		}
		else
		{
			std::println("[{}]: Dropped packet (invalid address).", net->get_primary_ip());
			std::string src = packet.src_ip();
			std::string dest = packet.dest_ip();
			proc.warnln("Discarding malformed package (invalid address).");
		}
	};

	auto frame_is_for_us = [our_mac = net->get_mac()](const ip::DataFrame& frame) -> bool
	{
		if (frame.dest_mac() == our_mac)
			return true;

		/* Broadcast address */
		if (frame.dest_mac() == uint64_t(-1))
			return true;

		return false;
	};

	/* 
	* 1. Check IP of incoming packet.
	* 2. If the IP is ours, look for a receiver in the sockets list.
	* 3. Otherwise dump it into the TX queue for the dispatcher to handle (or discard).
	*/

	while (true)
	{
		ip::DataFrame frame = co_await net->async_read_rx();

		if (not frame_is_for_us(frame))
		{
			proc.warnln("Discarding frame with incorrect address.");
			continue;
		}

		switch (frame.frame_type()) 
		{
			case ip::FrameType::ARP:
			{
				ip::ArpPackage pak;
				if (pak.ParseFromString(frame.payload()))
				{
					receive_arp(std::move(pak), frame.src_mac());
				}
				continue;
			}
			case ip::FrameType::IP:
			{
				ip::IpPackage pak;
				if (pak.ParseFromString(frame.payload()))
				{
					receive_ip(std::move(pak), frame.src_mac());
				}
				continue;
			}
		}
	}

	co_return 1;
}