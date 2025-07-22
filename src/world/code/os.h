#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "task.h"
#include "proc.h"
#include "timer_mgr.h"

class OS;
class Host;
class Process;
class World;

using process_args_t = std::function<ProcessTask(Proc, std::vector<std::string>)>;

struct Shell
{
	void exec(std::string cmd) {};
};

class OS
{
public:

	using schedule_fn = std::function<void()>;

	OS(Host& owner)
		: owner_(owner) { register_commands(); }


	/* Placeholder -- just statically add all known commands to command list. */
	virtual void register_commands();

	/* Execute a command in the OS environment. */
	void exec(std::string cmd);

	/* Shut down the host environment (and then the host). */
	void shutdown_os();

	/* Get the hostname from the owning Host. */
	[[nodiscard]] const std::string& get_hostname() const;

	/* Gets the world from the owning Host. */
	[[nodiscard]] World& get_world();

	[[nodiscard]] Shell get_shell(std::istream& in_stream, std::ostream& out_stream);

	EagerTask<int32_t> create_process(process_args_t program, std::vector<std::string> args, std::istream& is = std::cin, std::ostream& os = std::cout);

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
	std::unordered_map<int32_t, ProcessTask> processes_{};
	std::unordered_map<std::string, process_args_t> commands_{}; //bad, also remove include when remove this

};