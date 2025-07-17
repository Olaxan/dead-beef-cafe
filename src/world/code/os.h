#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "task.h"
#include "timer_mgr.h"

class OS;
class Host;
class Process;
class World;

using process_args_t = std::function<Task<int32_t>(OS*, std::vector<std::string>)>;

class OS
{
public:

	using schedule_fn = std::function<void()>;

	OS(Host& owner)
		: owner_(owner) { }

	/* Execute a command in the OS environment. */
	void exec(std::string cmd);

	/* Shut down the host environment (and then the host). */
	void shutdown_os();

	/* Get the hostname from the owning Host. */
	std::string get_hostname() const;

	/* Gets the world from the owning Host. */
	World& get_world();

	EagerTask<int32_t> create_process(process_args_t program, std::vector<std::string> args);

	void list_processes() const;


	void add_command(std::string cmd_name, process_args_t command)
	{
		commands_[cmd_name] = std::move(command);
	}


	TimerAwaiter wait(float seconds);
	void schedule(float seconds, schedule_fn callback);

protected:

	Host& owner_;
	int32_t pid_counter_{0};
	std::string hostname_ = {};
	std::chrono::steady_clock::time_point last_update_{};
	std::unordered_map<int32_t, Task<int32_t>> processes_{};
	std::unordered_map<std::string, process_args_t> commands_{}; //bad, also remove include when remove this

};