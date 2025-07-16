#pragma once

#include <vector>
#include <string>
#include <memory>

#include "device.h"

struct Command;

class World;
class File;
class OS;

class Host
{
public:

	Host() = delete;
	Host(World& world, std::string Hostname);

	std::string get_hostname() const { return hostname_; }
	OS& get_os() { return *os_; }
	World& get_world() { return world_; }

	void start_host();
	void shutdown_host();
	void boot_from(const File& boot_file);

	/* Starts threaded host process */
	//void launch();

	static std::unique_ptr<Host> create_skeleton_host(World& world, std::string hostname = {});

	template<typename T = Device, typename ...Args>
	T& create_device(Args&& ...args)
	{
		return static_cast<T&>(*devices_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...)));
	}

	template<typename T = OS, typename ...Args>
	T& create_os(Args&& ...args)
	{
		return *(os_ = std::make_unique<T>(*this, std::forward<Args>(args)...));
	}

private:

	World& world_;

	std::string hostname_ = {};
	std::unique_ptr<OS> os_{nullptr};
	std::vector<std::unique_ptr<Device>> devices_{};
	bool state_{false};

};