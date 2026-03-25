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


ProcessTask Programs::SrvNetTx(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	NetManager* net = os.get_network_manager();
	Address6 local_addr = net->get_primary_ip();

	FileAccessFlags flags = FileAccessFlags::Create | FileAccessFlags::Write | FileAccessFlags::Append;
	FileScope log{proc, "/var/log/tx.log", flags};
	proc.set_writer([log = std::move(log)](const Proc& wproc, const std::string& str)
	{
		std::ignore = log.write(str);
	});

	proc.putln("TX service running.");

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
			size_t bytes = packet.ByteSizeLong();

			if (auto exp_dest_ip = Address6::from_bytes(dest_ip_bytes))
			{
				const Address6& dest_addr = *exp_dest_ip;

				if (dest_addr == local_addr)
				{
					net->receive(std::move(packet));
					continue;
				}

				if (std::optional<Uid64> arp_entry = net->arp_lookup(dest_addr); arp_entry.has_value())
				{
					net->send(std::move(packet), *arp_entry);
					continue;
				}
				else
				{
					proc.putln("Performing ARP request (to find {})...", dest_addr);
					net->arp_request();
					net->route(std::move(packet));
					continue;
				}
			}
			else
			{
				proc.warnln("Failed to extract address: {}.", exp_dest_ip.error().what());
			}
		}
		catch (const std::exception& e)
		{
			proc.errln("fatal system error: {}.", e.what());
			co_return 2;
		}
	}

	co_return 1;
}