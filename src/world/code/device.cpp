#include "device.h"
#include "host.h"
#include "world.h"
#include "task.h"
#include "timer_mgr.h"

#include <coroutine>
#include <print>

Task<bool> Device::start_device(Host* owner)
{
	co_await owner->get_world().get_timer_manager().wait(1.f);
	set_state(DeviceState::PoweredOn);
	co_return true;
}

Task<bool> Device::shutdown_device(Host* owner)
{
	co_await owner->get_world().get_timer_manager().wait(0.5f);
	set_state(DeviceState::PoweredOff);
	co_return true;
}

void Device::set_state(DeviceState new_state)
{
	std::println("'{0}' changing state from {1} to {2}.", 
		get_device_id(), Device::get_state_name(state_), Device::get_state_name(new_state));
		
	state_ = new_state;
}

std::string Device::get_state_name(DeviceState state)
{
	switch (state)
	{
		case DeviceState::Disabled:
			return "DISABLED";
		case DeviceState::PoweredOff:
			return "POWER_OFF";
		case DeviceState::PoweredOn:
			return "POWER_ON";
		default:
			return "UNKNOWN";
	}
}
