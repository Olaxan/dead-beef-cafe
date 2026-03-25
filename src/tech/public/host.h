#pragma once

#include "device.h"
#include "os.h"

#include <vector>
#include <string>
#include <memory>

namespace world { class Host; }

struct GameServices;
struct Command;

class OS;
class World;
class File;

class Host
{
public:

	Host() = delete;
	Host(std::string Hostname);
	~Host();

	void init(GameServices* services);

	const std::string& get_hostname() const { return hostname_; }
	OS& get_os();
	DeviceState get_state() const { return state_; }

	bool start_host();
	bool shutdown_host();

	auto& get_devices() const { return devices_; }

	template<std::derived_from<Device> T, typename ...Args>
	T& create_device(Args&& ...args)
	{
		return static_cast<T&>(*devices_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...)));
	}

	/* Gets the first registered device that matches the specified type. */
	template <std::derived_from<Device> T>
	T* get_device() const
	{
		for (auto& dev : devices_)
		{
			if (T* cast = dynamic_cast<T*>(dev.get()))
				return cast;
		}

		return nullptr;
	}

	/* Gets a list of devices matching the specified type. */
	template <std::derived_from<Device> T>
	std::vector<T*> get_devices_of_type() const
	{
		std::vector<T*> out;
		for (auto& dev : devices_)
		{
			if (T* cast = dynamic_cast<T*>(dev.get()))
				out.push_back(cast);
		}

		return out;
	}

	void set_os(std::unique_ptr<OS>&& os);

	bool serialize(world::Host* to);
	bool deserialize(const world::Host& from);

private:
	
	GameServices* services_{nullptr};
	std::string hostname_ = {};
	std::unique_ptr<OS> os_{nullptr};
	std::vector<std::unique_ptr<Device>> devices_{};
	DeviceState state_{DeviceState::PoweredOff};

};