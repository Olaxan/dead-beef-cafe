#pragma once

#include "device.h"
#include <print>

class CPU : public Device
{
public:

	CPU() = default;
	CPU(float mhz) : mhz_(mhz) {}

	virtual void config_device(std::string_view cmd) override {};
	virtual std::string get_device_id() const override { return "Processor"; }
	virtual std::string get_driver_id() const override { return "cpu"; }

	float get_physical_speed() const { return mhz_; }
	void set_physical_speed(float mhz) { mhz_ = mhz; }

private:

	float mhz_ = 0.f;
};