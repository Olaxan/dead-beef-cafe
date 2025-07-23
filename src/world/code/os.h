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

	[[nodiscard]] Proc* get_shell(std::istream& in_stream = std::cin, std::ostream& out_stream = std::cout);
	Proc* create_process(std::istream& is = std::cin, std::ostream& os = std::cout);
	EagerTask<int32_t> create_process(process_args_t program, std::vector<std::string> args, std::istream& is = std::cin, std::ostream& os = std::cout);

	void list_processes() const;

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
			proc.write("No shell installed!");
			co_return 1; 
		};
	}

protected:

	Host& owner_;
	int32_t pid_counter_{0};
	std::string hostname_ = {};
	std::unordered_map<int32_t, Proc> processes_{};
	std::unordered_map<std::string, process_args_t> commands_{}; //bad, also remove include when remove this

	friend Proc;
};