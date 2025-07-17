#pragma once

#include "device.h"
#include <print>

class NIC : public Device
{
public:

	NIC() = default;
	NIC(float bandwidth) : bandwidth_(bandwidth) {}

	virtual void config_device(std::string_view cmd) override {};
	virtual std::string get_device_id() const override { return "Network Interface Card"; }

	void set_physical_bandwidth(float gbps) { bandwidth_ = gbps; }

private:

	float bandwidth_ = 0.f;

};