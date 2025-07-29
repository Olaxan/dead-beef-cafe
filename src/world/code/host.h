#pragma once

#include "device.h"
#include "os.h"

#include <vector>
#include <string>
#include <memory>

struct Command;

class World;
class File;

class Host
{
public:

	Host() = delete;
	Host(World& world, std::string Hostname);

	const std::string& get_hostname() const { return hostname_; }
	OS& get_os() { return *os_; }
	World& get_world() { return world_; }
	DeviceState get_state() const { return state_; }

	Task<bool> start_host();
	Task<bool> shutdown_host();
	Task<bool> boot_from(const File& boot_file);

	template<std::derived_from<Device> T, typename ...Args>
	T& create_device(Args&& ...args)
	{
		return static_cast<T&>(*devices_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...)));
	}

	template<std::derived_from<OS> T, typename ...Args>
	T& create_os(Args&& ...args)
	{
		os_ = std::move(std::make_unique<T>(*this, std::forward<Args>(args)...));
		return static_cast<T&>(*os_);
	}

private:
	
	World& world_;	
	std::string hostname_ = {};
	std::unique_ptr<OS> os_{nullptr};
	std::vector<std::unique_ptr<Device>> devices_{};
	DeviceState state_{DeviceState::PoweredOff};

};