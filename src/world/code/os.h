#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <concepts>

#include "task.h"
#include "proc.h"
#include "timer_mgr.h"
#include "netw.h"

class OS;
class Host;
class Process;
class World;
class ISocket;

class OS
{
public:

	using schedule_fn = std::function<void()>;

	OS(Host& owner)
		: owner_(owner) { register_commands(); }


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

	template<std::derived_from<ISocket> T>
	T* create_socket(int32_t port)
	{
		auto [it, success] = sockets_.emplace(port, std::make_unique<T>(port, this));
    	return success ? &it->second : nullptr;
	}

	template<typename T_Tx, typename T_Rx>
	Socket<T_Tx, T_Rx>* create_socket(int32_t port)
	{
		auto [it, success] = sockets_.emplace(port, std::make_unique<Socket<T_Tx, T_Rx>>(port, this));
    	return success ? &it->second : nullptr;
	}
	
	bool close_socket(int32_t port)
	{
		return sockets_.erase(port);
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
	std::string hostname_ = {};
	std::unordered_map<int32_t, Proc> processes_{};
	std::unordered_map<std::string, process_args_t> commands_{}; // This should probably be shared between all OS instances (static?)
	std::unordered_map<int32_t, std::unique_ptr<ISocket>> sockets_{};

	friend Proc;
};