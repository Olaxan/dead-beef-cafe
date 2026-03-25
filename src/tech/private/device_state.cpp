#include "device_state.h"

std::string DeviceUtils::get_state_name(DeviceState state)
{
	switch (state)
	{
		case DeviceState::Starting:
			return "STARTING";
		case DeviceState::Stopping:
			return "STOPPING";
		case DeviceState::Disabled:
			return "DISABLED";
		case DeviceState::PoweredOff:
			return "POWER_OFF";
		case DeviceState::PoweredOn:
			return "POWER_ON";
		case DeviceState::Error:
			return "ERROR";
		default:
			return "UNKNOWN";
	}
}