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

EagerTask<int32_t> ArpDelay(Proc& proc, ip::IpPackage&& packet, Address6 dest_addr)
{
	OS& os = *proc.owning_os;
	NetManager* net = os.get_network_manager();

	ip::IpPackage local_packet = std::move(packet);

	proc.putln("Placing {} bytes in ARP buffer.", local_packet.ByteSizeLong());
	proc.putln("Performing ARP request (to find {}).", dest_addr);

	ip::ArpPackage arp;
	arp.set_dest_ip(dest_addr.data_str());
	arp.set_type(ip::ArpType::PROBE);
	arp.set_src_ip(net->get_primary_ip().data_str());
	arp.set_src_mac(net->get_mac().num);

	ip::DataFrame frame;
	frame.set_dest_mac(MAC_BROADCAST);
	frame.set_src_mac(net->get_mac().num);
	frame.set_payload(arp.SerializeAsString());
	frame.set_frame_type(ip::FrameType::ARP);

	net->send(std::move(frame));

	for (std::size_t i = 0; i < 3; ++i)
	{
		float holdoff = static_cast<float>(1 << i) * 0.25f;
		proc.putln("Waiting {} seconds for ARP reply.", holdoff);
		co_await proc.wait(holdoff);

		if (std::optional<Uid64> arp_entry = net->arp_lookup(dest_addr); arp_entry.has_value())
		{
			proc.putln("Received ARP reply ({}), transmitting...", *arp_entry);
			net->send(std::move(local_packet), *arp_entry);
			co_return 0;
		}
		else
		{
			proc.putln("No ARP reply ({}/{}).", i + 1, 3);
		}
	}

	proc.putln("ARP request timed out (3 tries).");
	
	co_return 1;
}

ProcessTask Programs::SrvNetTx(Proc& proc, std::vector<std::string> args)
{
	const std::string log_path = "/var/log/tx.log";

	OS& os = *proc.owning_os;
	NetManager* net = os.get_network_manager();

	FileAccessFlags flags = FileAccessFlags::Create | FileAccessFlags::Write | FileAccessFlags::Append;
	FileScope log{proc, log_path, flags};
	proc.set_writer([log = std::move(log)](Proc& wproc, WriteInput str)
	{
		if (str) { std::ignore = log.write(*str); }
	});

	if (not net->get_primary_ip().is_valid())
	{
		Uid64 tail = Uid64Generator::generate();
		Address6 slaac_addr = Address6(os.get_uid().num, tail.num);
		net->set_primary_ip(slaac_addr);
	}

	proc.putln("TX service running.");
	proc.putln("Local ip: {}, mac={}.", net->get_primary_ip(), net->get_mac());
	proc.putln("Log file at {}.", log_path);

	net->register_node();
	proc.putln("Registered netnode.");

	while (true)
	{
		ip::IpPackage packet = co_await net->async_read_tx();

		if (auto exp_dest_ip = Address6::from_bytes(packet.dest_ip()))
		{
			const Address6& dest_addr = *exp_dest_ip;

			if (dest_addr == net->get_primary_ip())
			{
				net->unpack(std::move(packet));
				continue;
			}

			if (auto exp_next_addr = net->get_route(dest_addr))
			{
				Address6 next_addr = *exp_next_addr;

				proc.putln("Packet {} -via-> {}.", dest_addr, next_addr);

				if (std::optional<Uid64> arp_entry = net->arp_lookup(next_addr); arp_entry.has_value())
				{
					net->send(std::move(packet), *arp_entry);
					continue;
				}
				else
				{
					ArpDelay(proc, std::move(packet), next_addr);
					continue;
				}
			}
			else
			{
				proc.putln("Destination not found in routing table. Dropping.");
			}
		}
		else
		{
			proc.warnln("Failed to extract address: {}.", exp_dest_ip.error().what());
		}
	}

	net->unregister_node();
	proc.putln("Unregistered netnode.");
	/*TODO: This will probably crash on exit because we're not keeping track of our spawned coroutines!! Fix it!*/

	co_return 0;
}