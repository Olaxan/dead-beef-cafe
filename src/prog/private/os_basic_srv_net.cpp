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

	while (true)
	{
		ip::IpPackage packet = co_await rx_queue.async_read_one();
		/* 
		* 1. Check IP of incoming packet.
		* 2. If the IP is ours, look for a receiver in the sockets list.
		* 3. Otherwise dump it into the TX queue for the dispatcher to handle (or discard).
		*/

		std::string&& dest_ip_bytes = packet.dest_ip();
		if (dest_ip_bytes.size() != 16)
			continue;

		Address6 dest_ip = Address6(dest_ip_bytes.data());
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
		ip::IpPackage packet = co_await tx_queue.async_read_one();
		/* 
		* 1. Check IP of outgoing packet.
		* 2. If the IP is in the "ARP" cache, send it across the wire to the receiving "MAC".
		* 3. Otherwise, perform an ARP request and try again (maybe skip, increase fail count?).
		* 4. If fail count > N, discard package, return a rejection package to source.
		*/

		const std::string&& dest_ip_bytes = packet.dest_ip();
		if (dest_ip_bytes.size() != 16)
			continue;

		Address6 dest(dest_ip_bytes.data());
	}

	co_return 1;
}