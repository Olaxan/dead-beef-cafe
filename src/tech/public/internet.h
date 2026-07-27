#pragma once

#include "uid64.h"
#include "rel_mgr.h"

#include <proto/ip_packet.pb.h>

#include <unordered_map>

class Host;
class NetNode;

class Internet
{
public:

	Internet() = default;
	~Internet() = default;

	void register_node(Uid64 mac, NetNode* node);
	void unregister_node(Uid64 mac);

	void link(Uid64 from, Uid64 to);
	void unlink(Uid64 from, Uid64 to);
	void unlink_all(Uid64 node);

	int32_t distance(Uid64 from, Uid64 to);

	bool link_unicast(Uid64 from, Uid64 to, ip::IpPackage&& pak);
	int32_t link_broadcast(Uid64 from, ip::IpPackage&& pak);

protected:

	bool link_receive(Uid64 to, ip::IpPackage&& pak);

private:

	std::unordered_map<Uid64, NetNode*> nodes_;
	RelationManager<RelationshipType::OneToMany, Uid64, Uid64> links_;

};