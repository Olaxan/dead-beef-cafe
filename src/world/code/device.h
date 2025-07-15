#pragma once

#include <string_view>
#include <optional>

class File;

class Device
{
public:

	virtual void start_device() = 0;
	virtual void shutdown_device() = 0;
	virtual void config_device(std::string_view cmd) = 0;
};