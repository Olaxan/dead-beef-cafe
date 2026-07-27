#pragma once

#include "device.h"
#include "addr.h"
#include "net_types.h"

#include "proto/ip_packet.pb.h"

#include <print>
#include <functional>
#include <vector>


class NIC : public Device
{
public:

	NIC() = default;

	NIC(float bandwidth) : bandwidth_(bandwidth) { }

	~NIC();

	virtual void config_device(std::string_view cmd) override {};
	virtual std::string get_device_id() const override { return "Network Interface Card"; }
	virtual std::string get_driver_id() const override { return "net"; }

	float get_physical_bandwidth() const { return bandwidth_; }
	void set_physical_bandwidth(float gbps) { bandwidth_ = gbps; }

	virtual void on_start(Host* owner) override;
	virtual void on_shutdown(Host* owner) override;

protected:

	float bandwidth_ = 0.f;

};