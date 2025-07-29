#pragma once

#include "task.h"
#include "proc.h"
#include "timer_mgr.h"
#include "netw.h"
#include "ip_mgr.h"
#include "world.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <concepts>

class Host;

class OS
{
public:

	using schedule_fn = std::function<void()>;

	OS() = delete;

	OS(Host& owner)
		: owner_(owner) { register_commands(); }

	virtual ~OS() { }

	/* Placeholder -- just statically add all known commands to command list. */
	virtual void register_commands() {};

	/* Shut down the host environment (and then the host). */
	void shutdown_os();

	/* Get the hostname from the owning Host. */
	[[nodiscard]] const std::string& get_hostname() const;

	/* Gets the world from the owning Host. */
	[[nodiscard]] World& get_world();

	[[nodiscard]] Proc* get_shell(std::ostream& out_stream = std::cout);
	Proc* create_process(std::ostream& os = std::cout);
	EagerTask<int32_t> create_process(process_args_t program, std::vector<std::string> args, std::ostream& os = std::cout);
	void list_processes() const;


	/* Sockets */
	template<std::derived_from<ISocket> T>
	std::shared_ptr<T> create_socket()
	{
		// int32_t fd = ++fd_counter_;
		// auto [it, success] = sockets_.emplace(fd, std::make_shared<T>());
    	// return success ? *it : nullptr;
		return std::make_shared<T>();
	}

	template<typename T_Tx, typename T_Rx>
	std::shared_ptr<Socket<T_Tx, T_Rx>> create_socket()
	{
		int32_t fd = ++fd_counter_;
		auto [it, success] = sockets_.emplace(fd, std::make_shared<Socket<T_Tx, T_Rx>>());
    	return success ? *it : nullptr;
	}
	
	bool close_socket(int32_t fd)
	{
		return sockets_.erase(fd);
	}

	/* Placeholder */
	Address6 get_global_ip() { return {1}; }

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


	void add_program(const std::string& cmd_name, process_args_t command)
	{
		commands_[cmd_name] = std::move(command);
	}

	TimerAwaiter wait(float seconds);
	void schedule(float seconds, schedule_fn callback);

protected:

	[[nodiscard]] auto get_program(const std::string& cmd)
	{ 
		auto it = commands_.find(cmd);
		return std::pair{it, it != commands_.end() };
	}

	[[nodiscard]] process_args_t get_default_shell() const
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
	int32_t fd_counter_{0};
	std::string hostname_ = {};
	std::unordered_map<int32_t, Proc> processes_{};
	std::unordered_map<int32_t, std::shared_ptr<ISocket>> sockets_;
	std::unordered_map<std::string, process_args_t> commands_{}; // This should probably be shared between all OS instances (static?)

	friend Proc;
};