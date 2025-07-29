#pragma once

#include <coroutine>
#include <string_view>

#include "task.h"

class Device;
class Host;

enum class DeviceState
{
	PoweredOff,
	PoweredOn,
	Starting,
	Stopping,
	Disabled,
	Error
};

class Device
{
public:

	virtual Task<bool> start_device(Host* owner);
	virtual Task<bool> shutdown_device(Host* owner);
	virtual void config_device(std::string_view cmd) {};
	
	virtual std::string get_device_id() const { return "Generic Device"; }

	DeviceState get_state() { return state_; }
	void set_state(DeviceState new_state);

	virtual void on_start(Host* owner) {};
	virtual void on_shutdown(Host* owner) {};

	static std::string get_state_name(DeviceState state);

	virtual ~Device() { }

protected:

	DeviceState state_{ DeviceState::PoweredOff };

};