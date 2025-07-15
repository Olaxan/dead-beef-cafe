#pragma once

#include "device.h"

class CPU : public Device
{
public:

	CPU() = default;
	CPU(float mhz) : mhz_(mhz) {}

	virtual void start_device() override {};
	virtual void shutdown_device() override {};
	virtual void config_device(std::string_view cmd) override {};

	void set_physical_speed(float mhz) { mhz_ = mhz; }

private:

	float mhz_ = 0.f;
};