#include "net_mgr.h"
#include "uuid.h"

#include "proto/ip_packet.pb.h"

bool NetManager::test_reach(Address6 remote)
{
	// Address6&& src = get_local_address();

	// if (remote == src)
	// 	return true;

	// ip::IpPackage pack{};
	// pack.set_dest_ip(remote);
	// pack.set_src_ip(src);
	// pack.set_protocol(ip::Protocol::ICMP);

	// std::optional<UUID> mac = arp_query();

	// if (!mac)
	// 	return false;
	
	return false;
}