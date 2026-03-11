#pragma once

#include "device.h"
#include "endpoint.h"
#include "addr.h"
#include "net_types.h"
#include "link_srv.h"

#include "proto/ip_packet.pb.h"

#include <print>
#include <functional>
#include <vector>


using NetCastFn = std::function<void(Uid64, class NIC*)>;

class NIC : public Device, public IEndpoint, public ILinkable
{
public:

	NIC() = default;

	NIC(float bandwidth) 
		: bandwidth_(bandwidth)
	{
		set_ip(IPv6Generator::generate());	
	}

	~NIC();

	virtual void config_device(std::string_view cmd) override {};
	virtual std::string get_device_id() const override { return "Network Interface Card"; }
	virtual std::string get_driver_id() const override { return "net"; }

	float get_physical_bandwidth() const { return bandwidth_; }
	void set_physical_bandwidth(float gbps) { bandwidth_ = gbps; }

	/* Endpoint IF */
	void set_ip(const std::string& new_ip) override;
	void set_ip(const Address6& new_ip) override { address_ = new_ip; }
	const Address6& get_ip() const override { return address_; }
	/* Endpoint IF */

	/* Linkable IF */
	void on_linked(LinkServer* links, ILinkable* other) override;
	void on_unlinked(LinkServer* links, ILinkable* other) override;
	/* Linkable IF */

	virtual void on_start(Host* owner) override;
	virtual void on_shutdown(Host* owner) override;

	size_t transfer(Uid64 mac, ip::IpPackage&& packet);
	void broadcast(NetCastFn broadcast_fn);
	void unicast(Uid64 mac, NetCastFn unicast_fn);

	void add_link_update_callback(LinkUpdateCallbackFn&& fn);
	void notify_link_update(Uid64 new_mac, LinkUpdateType type);

	NetQueue& get_rx_queue() { return rx_queue_; }
	NetQueue& get_tx_queue() { return tx_queue_; }

protected:

	std::vector<LinkUpdateCallbackFn> callbacks_{};
	std::unordered_map<Uid64, NIC*> link_cache_{};

	Address6 address_{};
	float bandwidth_ = 0.f;

	NetQueue rx_queue_{};
	NetQueue tx_queue_{};

};