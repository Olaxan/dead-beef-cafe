#include "os_basic.h"

#include "os.h"
#include "netw.h"
#include "filesystem.h"

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
	MessageQueue<ip::IpPackage>& rx_queue = net->get_rx_queue();
	MessageQueue<ip::IpPackage>& tx_queue = net->get_tx_queue();
	Address6 local_ip = os.get_global_ip();

	auto handle_packet = [&os](ip::IpPackage& packet)
	{
		switch (packet.protocol())
		{
			case ip::Protocol::TCP:
			{

			}
			case ip::Protocol::UDP:
			{

			}
			case ip::Protocol::ICMP:
			{

			}
		}
	};

	while (true)
	{
		ip::IpPackage packet = co_await rx_queue.async_read();
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
				std::println("({}) We got a packet addressed to us!", dest_ip);
			}
			else
			{
				tx_queue.push(std::move(packet));
			}
		}
	}

	co_return 1;
}

ProcessTask Programs::SrvNetTx(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	NetManager* net = os.get_network_manager();
	MessageQueue<ip::IpPackage>& tx_queue = net->get_tx_queue();

	while (true)
	{
		/* 
		* 1. Check IP of outgoing packet.
		* 2. If the IP is in the "ARP" cache, send it across the wire to the receiving "MAC".
		* 3. Otherwise, perform an ARP request and try again (maybe skip, increase fail count?).
		* 4. If fail count > N, discard package, return a rejection package to source.
		*/

		try
		{
			ip::IpPackage packet = co_await tx_queue.async_read();
			const std::string& dest_ip_bytes = packet.dest_ip();

			if (auto exp_dest_ip = Address6::from_bytes(dest_ip_bytes))
			{
				const Address6& dest_ip = *exp_dest_ip;
				std::println("Sent {} bytes (dest. {}).", packet.ByteSizeLong(), dest_ip);
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