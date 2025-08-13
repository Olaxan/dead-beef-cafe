#include "device.h"
#include "host.h"
#include "world.h"
#include "timer_mgr.h"

#include <print>

bool Device::start_device(Host* owner)
{
	if (state_ == DeviceState::PoweredOn)
		return false;

	set_state(DeviceState::PoweredOn);
	on_start(owner);
	return true;
}

bool Device::shutdown_device(Host* owner)
{
	if (state_ == DeviceState::PoweredOff)
		return false;

	set_state(DeviceState::PoweredOff);
	on_shutdown(owner);
	return true;
}
