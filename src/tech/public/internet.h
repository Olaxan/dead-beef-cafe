#pragma once

#include "uid64.h"
#include "rel_mgr.h"

#include <proto/dataframe.pb.h>

#include <unordered_map>
#include <vector>
#include <tuple>
#include <memory>

class Host;
struct NetNode;

using NetPathResult = std::pair<bool, std::vector<Uid64>>;

class Internet
{
public:

	Internet() = default;
	~Internet() = default;

	void register_node(Uid64 mac, std::shared_ptr<NetNode> node);
	void unregister_node(Uid64 mac);

	void link(Uid64 from, Uid64 to);
	void unlink(Uid64 from, Uid64 to);
	void unlink_all(Uid64 node);

	NetPathResult get_path(Uid64 from, Uid64 to) const;
	size_t distance(Uid64 from, Uid64 to) const;

	bool link_unicast(ip::DataFrame&& pak);
	int32_t link_broadcast(ip::DataFrame&& pak);

protected:

	bool link_receive(Uid64 to, ip::DataFrame&& pak);

private:

	std::unordered_map<Uid64, std::shared_ptr<NetNode>> nodes_;
	RelationManager<RelationshipType::ManyToMany, Uid64, Uid64> links_;

};