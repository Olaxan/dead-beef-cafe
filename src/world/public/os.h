#pragma once

//#include "host.h"
#include "task.h"
#include "proc.h"
#include "timer_mgr.h"
#include "netw.h"
#include "net_mgr.h"
#include "world.h"
#include "device.h"
#include "device_state.h"
#include "session.h"
#include "session_mgr.h"
#include "users_mgr.h"
#include "net_srv.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <concepts>

//class Host;
class FileSystem;
class Host;
class World;

class OS
{
public:

	using SchedulerFn = std::function<void()>;

	OS() = delete;

	OS(Host& owner);

	virtual ~OS();

	virtual std::size_t register_devices();

	/* Start the host environment (and then the host). */
	virtual void start_os();

	/* Shut down the host environment (and then the host). */
	virtual void shutdown_os();

	/* Get the hostname from the owning Host. */
	[[nodiscard]] const std::string& get_hostname() const;

	/* Get the os device state. */
	[[nodiscard]] DeviceState get_state() const { return state_; }
	void set_state(DeviceState new_state) { state_ = new_state; }

	/* Get the uuid->device reference map. */
	[[nodiscard]] auto& get_devices() { return devices_; }

	/* Gets the world from the owning Host. */
	[[nodiscard]] World& get_world();

	/* Gets the owning host of the OS. */
	[[nodiscard]] Host& get_owner();

	/* Gets the filesystem, if one exists (otherwise nullptr). */
	[[nodiscard]] FileSystem* get_filesystem() const;

	/* Gets the users/auth manager. */
	[[nodiscard]] UsersManager* get_users_manager();

	/* Gets the session manager. */
	[[nodiscard]] SessionManager* get_session_manager();

	/* Gets the network manager. */
	[[nodiscard]] NetManager* get_network_manager();

	struct CreateProcessParams
	{
		WriterFn writer{nullptr};
		ReaderFn reader{nullptr};
		int32_t leader_id{-1};
		int32_t uid{0};
		int32_t gid{0};
	};

	Proc* create_process(CreateProcessParams&& params = {});
	EagerTask<int32_t> run_process(ProcessFn program, std::vector<std::string> args, CreateProcessParams&& params = {});
	void get_processes(std::function<void(const Proc&)> reader) const;

	int32_t create_sid();


	/* Device management */

	/* Gets the first registered device that matches the specified type. */
	template <std::derived_from<Device> T>
	T* get_device() const
	{
		for (auto& [id, dev] : devices_)
		{
			if (T* cast = dynamic_cast<T*>(dev))
				return cast;
		}

		return nullptr;
	}

	/* Gets a list of devices matching the specified type. */
	template <std::derived_from<Device> T>
	std::vector<T*> get_devices_of_type() const
	{
		std::vector<T*> out;
		for (auto& [id, dev] : devices_)
		{
			if (T* cast = dynamic_cast<T*>(dev))
				out.push_back(cast);
		}

		return out;
	}

	/* --- Scheduler --- */

	[[nodiscard]] TimerAwaiter wait(float seconds);
	void schedule(float seconds, SchedulerFn callback);


protected:

	Host& owner_;
	int32_t pid_counter_{0};
	int32_t fd_counter_{0};
	std::string hostname_ = {};
	DeviceState state_{DeviceState::PoweredOff};
	std::unordered_map<int32_t, Device*> devices_{};
	std::unordered_map<int32_t, std::unique_ptr<Proc>> processes_{};

	UsersManager users_{this};
	SessionManager sess_{this};
	NetManager net_{this};

	friend Proc;
};