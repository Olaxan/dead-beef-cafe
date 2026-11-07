#include "prog_basic.h"

#include "os.h"
#include "net_types.h"
#include "link_awaiter.h"
#include "filesystem.h"
#include "net_mgr.h"
#include "scoped_fd.h"

#include "proto/ip_packet.pb.h"
#include "proto/arp_packet.pb.h"

#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <ranges>
#include <functional>
#include <tuple>

#include <iso646.h>


ProcessTask Programs::SrvNetArp(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	NetManager* net = os.get_network_manager();
	Address6 local_addr = net->get_primary_ip();

	FileAccessFlags flags = FileAccessFlags::Create | FileAccessFlags::Write | FileAccessFlags::Append;
	FileScope log{proc, "/var/log/arp.log", flags};
	proc.set_writer([log = std::move(log)](Proc& wproc, WriteInput str)
	{
		if (str) { std::ignore = log.write(*str); }
	});

	proc.putln("ARP service running.");

	while (true)
	{
		ip::IpPackage packet = co_await net->async_read_arp();

		ip::ArpPackage arp;
		

	}

	co_return 0;
}