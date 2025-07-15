#pragma once

#include "device.h"

class NIC : public Device
{
public:

	NIC() = default;
	NIC(float bandwidth) : bandwidth_(bandwidth) {}

	virtual void start_device() override {};
	virtual void shutdown_device() override {};
	virtual void config_device(std::string_view cmd) override {};

	void set_physical_bandwidth(float gbps) { bandwidth_ = gbps; }

private:

	float bandwidth_ = 0.f;

};