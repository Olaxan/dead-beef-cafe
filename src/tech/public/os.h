#pragma once

#include "task.h"
#include "proc.h"
#include "timer_awaiter.h"
#include "net_types.h"
#include "net_mgr.h"
#include "device.h"
#include "device_state.h"
#include "session.h"
#include "session_mgr.h"
#include "users_mgr.h"
#include "game_srv.h"
#include "filesystem.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <concepts>
#include <any>

struct GameServices;

namespace world { class Host; }

class OS
{
public:

	using SchedulerFn = std::function<void()>;

	OS();
	virtual ~OS();

	void init(GameServices* services);

	virtual std::size_t register_devices();

	/* Start the host environment (and then the host). */
	virtual void start_os();

	/* Shut down the host environment (and then the host). */
	virtual void shutdown_os();

	/* Get the hostname of the OS. */
	[[nodiscard]] std::string_view get_hostname() const;

	/* Get the os device state. */
	[[nodiscard]] DeviceState get_state() const { return state_; }
	void set_state(DeviceState new_state) { state_ = new_state; }

	/* Gets the filesystem. */
	[[nodiscard]] FileSystem* get_filesystem();

	/* Gets the users/auth manager. */
	[[nodiscard]] UsersManager* get_users_manager();

	/* Gets the session manager. */
	[[nodiscard]] SessionManager* get_session_manager();

	/* Gets the network manager. */
	[[nodiscard]] NetManager* get_network_manager();

	/* Gets the services struct. */
	[[nodiscard]] GameServices* get_services();

	/* Gets the audio interface. */
	[[nodiscard]] IAudioBase* get_audio();

	template <typename T>
	[[nodiscard]] T get_outer_as()
	{
		if (services_ == nullptr)
			return nullptr;

		try
		{
			return std::any_cast<T>(services_->outer);
		}
		catch (const std::bad_any_cast&)
		{
			return nullptr;
		}
	}

	struct CreateProcessParams
	{
		InvokeFn invoke{nullptr};
		WriterFn writer{nullptr};
		ReaderFn reader{nullptr};
		int32_t fork_pid{-1};
		int32_t leader_id{-1};
		int32_t uid{0};
		int32_t gid{0};
		bool tty{true};
	};

	Proc* create_process(CreateProcessParams&& params = {});
	void kill_process(int32_t pid);
	EagerTask<int32_t> run_process(ProcessFn program, std::vector<std::string> args, CreateProcessParams&& params = {});
	void get_processes(std::function<void(const Proc&)> reader) const;
	bool process_is_running(int32_t pid) const;

	int32_t create_sid();

	/* --- Scheduler --- */

	[[nodiscard]] TimerAwaiter wait(float seconds);
	void schedule(float seconds, SchedulerFn callback);

	bool serialize(world::Host* to);
	bool deserialize(const world::Host& from);

protected:

	GameServices* services_{nullptr};

	int32_t pid_counter_{0};
	int32_t fd_counter_{0};
	std::string hostname_ = {};
	DeviceState state_{DeviceState::PoweredOff};
	std::unordered_map<int32_t, Device*> devices_{};
	std::unordered_map<int32_t, std::unique_ptr<Proc>> processes_{};

	UsersManager users_{this};
	SessionManager sess_{this};
	NetManager net_{this};
	FileSystem fs_{this};

	friend Proc;
};