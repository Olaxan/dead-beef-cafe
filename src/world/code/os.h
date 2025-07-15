#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "task.h"
#include "msg_queue.h"

class Host;
class Process;

class OS
{
public:

	using schedule_fn = std::function<void()>;

	OS(Host& owner)
		: owner_(owner) { }

	/* Execute a command in the OS environment. */
	void exec(std::string cmd);

	/* Run a full process cycle (run the scheduler). */
	void run();

	/* Shut down the host environment (and then the host). */
	void shutdown_os();

	/* Get the hostname from the owning Host. */
	std::string get_hostname() const;

	/* Get the os message queue. */
	MessageQueue& get_msg_queue() { return msg_queue_; }

	template<typename T = Process>
	int32_t create_process(process_entry_t program)
	{
		int32_t pid = pid_counter_++;
		auto exit_callback = [this](int32_t pid, int32_t ec) { processes_.erase(pid); };
		auto new_process = std::make_unique<T>(pid, program, exit_callback);
		processes_[pid] = std::move(new_process);
		return pid;
	}

	void add_command(std::string cmd_name, process_args_t command)
	{
		commands_[cmd_name] = std::move(command);
	}

	struct AsyncTimeAwaiter
	{
		explicit AsyncTimeAwaiter(OS* os, float seconds)
			: os_(os), time_(seconds) { }

		bool await_ready() const { return false; }
		void await_suspend(std::coroutine_handle<> h) const 
		{
			os_->schedule(time_, [h]{
				h.resume();
			});
		};

		void await_resume() const {}

	private:

		OS* os_;
		float time_{0};

	};

	AsyncTimeAwaiter wait(float seconds) 
	{ 
		return AsyncTimeAwaiter{this, seconds};
	};

	void schedule(float seconds, schedule_fn callback) 
	{
		task_queue_.emplace_back(seconds, std::move(callback));
	}

protected:

	Host& owner_;
	int32_t pid_counter_{0};
	std::string hostname_ = {};
	MessageQueue msg_queue_{};
	std::chrono::steady_clock::time_point last_update_{};
	std::vector<std::pair<float, schedule_fn>> task_queue_{};
	std::unordered_map<int32_t, std::coroutine_handle<>> processes_{};
	std::unordered_map<std::string, process_args_t> commands_{}; //bad, also remove include when remove this

};