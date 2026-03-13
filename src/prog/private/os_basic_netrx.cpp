#include "os_basic.h"

#include "os.h"
#include "net_types.h"
#include "link_awaiter.h"
#include "filesystem.h"
#include "net_mgr.h"
#include "scoped_fd.h"

#include "proto/ip_packet.pb.h"

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
	Address6 local_ip = net->get_primary_ip();

	FileAccessFlags flags = FileAccessFlags::Create | FileAccessFlags::Write | FileAccessFlags::Append;
	FileScope log{proc, "/var/log/rx.log", flags};
	proc.set_writer([&proc, log = std::move(log)](const std::string& str)
	{
		std::ignore = log.write(str);
	});

	proc.putln("RX service running.");

	/* 
	* 1. Check IP of incoming packet.
	* 2. If the IP is ours, look for a receiver in the sockets list.
	* 3. Otherwise dump it into the TX queue for the dispatcher to handle (or discard).
	*/

	while (true)
	{
		ip::IpPackage packet = co_await net->async_read_rx();
		size_t packet_size = packet.ByteSizeLong();

		const std::string& src_ip_bytes = packet.src_ip();
		const std::string& dest_ip_bytes = packet.dest_ip();

		auto exp_src_addr = Address6::from_bytes(src_ip_bytes.data());
		auto exp_dest_addr = Address6::from_bytes(dest_ip_bytes.data());

		if (exp_src_addr && exp_dest_addr)
		{
			const Address6& src_ip = *exp_src_addr;
			const Address6& dest_ip = *exp_dest_addr;

			if (dest_ip == local_ip)
			{
				proc.putln("Receiving {} bytes (dest. {})...", packet_size, dest_ip);
				net->receive(std::move(packet), src_ip, dest_ip);
			}
			else
			{
				proc.putln("Forwarding {} bytes (dest. {})...", packet_size, dest_ip);
				net->send(std::move(packet));
			}
		}
		else
		{
			proc.warnln("Discarding malformed package (invalid address).");
		}
	}

	co_return 1;
}