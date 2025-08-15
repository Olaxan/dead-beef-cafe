#pragma once

#include "task.h"
#include "proc.h"
#include "timer_mgr.h"
#include "netw.h"
#include "ip_mgr.h"
#include "world.h"
#include "device.h"
#include "device_state.h"
#include "session.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <concepts>

class Host;
class FileSystem;

class OS
{
public:

	using schedule_fn = std::function<void()>;

	OS() = delete;

	OS(Host& owner)
		: owner_(owner) 
	{ 
		register_devices(); 
	}

	virtual ~OS() { }

	virtual std::size_t register_devices();

	/* Shut down the host environment (and then the host). */
	void shutdown_os();

	/* Get the hostname from the owning Host. */
	[[nodiscard]] const std::string& get_hostname() const;

	/* Get the os device state. */
	[[nodiscard]] DeviceState get_state() const { return state_; }
	void set_state(DeviceState new_state) { state_ = new_state; }

	/* Get the uuid->device reference map. */
	[[nodiscard]] auto& get_devices() { return devices_; }

	/* Gets the world from the owning Host. */
	[[nodiscard]] World& get_world();

	[[nodiscard]] Proc* get_shell(std::ostream& out_stream = std::cout);
	Proc* create_process(std::ostream& os = std::cout);
	Proc* create_process(Proc* host);
	EagerTask<int32_t> create_process(ProcessFn program, std::vector<std::string> args, std::ostream& os = std::cout);
	EagerTask<int32_t> create_process(ProcessFn program, std::vector<std::string> args, Proc* proc);
	void get_processes(std::function<void(const Proc&)> reader) const;


	/* Device management */

	/* Gets the first registered device that matches the specified type. */
	template <std::derived_from<Device> T>
	T* get_device() const
	{
		for (auto& [uuid, dev] : devices_)
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
		for (auto& [uuid, dev] : devices_)
		{
			if (T* cast = dynamic_cast<T*>(dev))
				out.push_back(cast);
		}

		return out;
	}

	FileSystem* get_filesystem() const;

	/* Sockets */
	template<std::derived_from<ISocket> T>
	std::shared_ptr<T> create_socket()
	{
		int32_t fd = ++fd_counter_;
		std::shared_ptr<T> new_ptr = std::make_shared<T>();
		sockets_[fd] = new_ptr;
		return new_ptr;
	}

	template<typename T_Tx, typename T_Rx>
	std::shared_ptr<Socket<T_Tx, T_Rx>> create_socket()
	{
		return create_socket<Socket<T_Tx, T_Rx>>();
	}
	
	bool close_socket(int32_t fd)
	{
		return sockets_.erase(fd);
	}

	/* Placeholder */
	Address6 get_global_ip();

	bool bind_socket(const std::shared_ptr<ISocket>& sock, int32_t listen_port)
	{
		IpManager& internet = get_world().get_ip_manager();
		return internet.bind(sock, get_global_ip(), listen_port);
	}

	template<std::derived_from<ISocket> TargetT>
	bool connect_socket(const std::shared_ptr<ISocket>& sock, Address6 dest_ip, int32_t dest_port)
	{
		IpManager& internet = get_world().get_ip_manager();
    	std::optional<SessionId> sid = internet.connect<TargetT>(sock, dest_ip, dest_port);
		return sid.has_value();
	}

	/* --- Sessions ---*/

	/* Creates a new user session and returns the sid. */
	int32_t create_session(int32_t uid = -1, int32_t gid = -1);
	
	/* Ends the specified session. Returns whether successful. */
	bool end_session(int32_t sid);

	/* Sets the uid of a session. Returns whether successful. */
	bool set_session_uid(int32_t sid, int32_t new_uid);

	/* Sets the gid of a session. Returns whether successful. */
	bool set_session_gid(int32_t sid, int32_t new_gid);

	/* Add supplementary groups to a session. Returns whether any groups were added. */
	bool add_session_groups(int32_t sid, const std::unordered_set<int32_t>& groups);

	/* Get a reference to a active session, if one matches the sid. */
	std::optional<SessionData> get_session(int32_t sid);

	/* --- Scheduler --- */

	[[nodiscard]] TimerAwaiter wait(float seconds);
	void schedule(float seconds, schedule_fn callback);



protected:

	[[nodiscard]] virtual ProcessFn get_default_shell() const
	{
		return [](Proc& proc, std::vector<std::string> args) -> ProcessTask 
		{ 
			proc.putln("No shell installed!");
			co_return 1; 
		};
	}

protected:

	Host& owner_;
	int32_t pid_counter_{0};
	int32_t sid_counter_{0};
	int32_t fd_counter_{0};
	std::string hostname_ = {};
	DeviceState state_{DeviceState::PoweredOff};
	std::unordered_map<int32_t, Device*> devices_{};
	std::unordered_map<int32_t, std::unique_ptr<Proc>> processes_{};
	std::unordered_map<int32_t, std::shared_ptr<ISocket>> sockets_;
	std::unordered_map<int32_t, SessionData> sessions_{};

	friend Proc;
};