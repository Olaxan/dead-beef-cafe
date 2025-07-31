#pragma once

#include "task.h"
#include "device_state.h"

#include <coroutine>
#include <string_view>


class Host;

class Device
{
public:

	virtual Task<bool> start_device(Host* owner);
	virtual Task<bool> shutdown_device(Host* owner);
	virtual void config_device(std::string_view cmd) {};
	
	virtual std::string get_device_id() const { return "Generic Device"; }
	virtual std::string get_driver_id() const { return "none"; }

	DeviceState get_state() { return state_; }
	void set_state(DeviceState new_state);

	virtual void on_start(Host* owner) {};
	virtual void on_shutdown(Host* owner) {};

	virtual ~Device() { }

protected:

	DeviceState state_{ DeviceState::PoweredOff };

};