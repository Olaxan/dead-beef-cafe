#pragma once

#include "device.h"
#include "endpoint.h"

#include <print>

class NIC : public Device, public IEndpoint
{
public:

	NIC() = default;
	NIC(float bandwidth) : bandwidth_(bandwidth) {}

	virtual void config_device(std::string_view cmd) override {};
	virtual std::string get_device_id() const override { return "Network Interface Card"; }

	void set_physical_bandwidth(float gbps) { bandwidth_ = gbps; }

	/* Endpoint IF */
	void set_ip(const std::string& new_ip) override { ip_ = new_ip; }
	const std::string& get_ip() const override { return ip_; }
	/* Endpoint IF */

	virtual void on_start(Host* owner) override;
	virtual void on_shutdown(Host* owner) override;

private:

	std::string ip_{"0.0.0.0"};
	float bandwidth_ = 0.f;

};