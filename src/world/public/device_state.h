#pragma once

#include <string>

enum class DeviceState
{
	PoweredOff,
	PoweredOn,
	Starting,
	Stopping,
	Disabled,
	Error
};

namespace DeviceUtils
{
	std::string get_state_name(DeviceState state);
}