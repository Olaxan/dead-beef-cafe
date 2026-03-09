#include "os_basic.h"

#include "os.h"
#include "netw.h"
#include "filesystem.h"
#include "net_mgr.h"

#include "proto/ip_packet.pb.h"

#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <ranges>
#include <functional>

ProcessTask Programs::SrvNetRx(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	NetManager* net = os.get_network_manager();
	Address6 local_ip = net->get_primary_ip();

	std::println("RX service running.");

	while (true)
	{
		ip::IpPackage packet = co_await net->async_read_rx();
		/* 
		* 1. Check IP of incoming packet.
		* 2. If the IP is ours, look for a receiver in the sockets list.
		* 3. Otherwise dump it into the TX queue for the dispatcher to handle (or discard).
		*/

		const std::string& dest_ip_bytes = packet.dest_ip();
		if (auto exp_dest_ip = Address6::from_bytes(dest_ip_bytes.data()))
		{
			const Address6& dest_ip = *exp_dest_ip;
			if (dest_ip == local_ip)
			{
				net->receive(std::move(packet), dest_ip);
			}
			else
			{
				net->send(std::move(packet));
			}
		}
	}

	co_return 1;
}

ProcessTask Programs::SrvNetTx(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	NetManager* net = os.get_network_manager();
	Address6 local_addr = net->get_primary_ip();

	std::println("TX service running.");

	while (true)
	{
		/* 
		* 1. Check IP of outgoing packet.
		* 2. If the IP is in the "ARP" cache, send it across the wire to the receiving "MAC".
		* 3. Otherwise, perform an ARP request and try again (maybe skip, increase fail count?).
		* 4. If fail count > N, discard package, maybe return a rejection package to source.
		*/

		try
		{
			ip::IpPackage packet = co_await net->async_read_tx();
			const std::string& dest_ip_bytes = packet.dest_ip();

			if (auto exp_dest_ip = Address6::from_bytes(dest_ip_bytes))
			{
				const Address6& dest_addr = *exp_dest_ip;

				if (dest_addr == local_addr)
				{
					std::println("Received {} bytes (dest. {}).", packet.ByteSizeLong(), dest_addr);
					net->receive(std::move(packet));
					continue;
				}
				
				std::println("Sent {} bytes (dest. {}).", packet.ByteSizeLong(), dest_addr);
			}
			else
			{
				std::println("Failed to extract address: {}.", exp_dest_ip.error().what());
			}
		}
		catch (const std::exception& e)
		{
			std::println("fatal system error: {}.", e.what());
			co_return 2;
		}
	}

	co_return 1;
}

ProcessTask Programs::SrvNetArp(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	NetManager* net = os.get_network_manager();
	Address6 local_addr = net->get_primary_ip();

	co_return 0;
}
