#pragma once

#include "device.h"
#include "endpoint.h"
#include "addr.h"

#include <print>

class NIC : public Device, public IEndpoint
{
public:

	NIC() = default;

	NIC(float bandwidth) 
		: bandwidth_(bandwidth) 
	{
		set_ip(IPv6Generator::generate());	
	}

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

	virtual void on_start(Host* owner) override;
	virtual void on_shutdown(Host* owner) override;

private:

	Address6 address_{};
	float bandwidth_ = 0.f;

};