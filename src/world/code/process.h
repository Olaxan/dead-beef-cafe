#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <coroutine>

class OS;

using process_entry_t = std::function<bool(OS& os, std::string_view cmd)>;
using process_event_t = std::function<void(int32_t, int32_t)>;

struct ProcessTask
{
	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

	struct promise_type
	{
		ProcessTask get_return_object() 
		{ 
			return 
			{
				.h = handle_type::from_promise(*this)
			}; 
		}

		std::suspend_never initial_suspend() { return {}; }
		std::suspend_never final_suspend() noexcept { return {}; }
		void return_void() {}
		void unhandled_exception() {}
	};

	handle_type h;

	operator handle_type() const { return h; }
  	// A coroutine_handle<promise_type> converts to coroutine_handle<>
	operator std::coroutine_handle<>() const { return h; }
};

using process_args_t = std::function<ProcessTask(OS&, std::vector<std::string>)>;

class Process
{
public:

	Process() = default;
	Process(int32_t pid, process_event_t on_exit = nullptr) 
		: pid_(pid), on_exit_(on_exit) {}

	//virtual void create_process() = 0;
	
	void run_process(float delta_time)
	{
		//std::invoke(on_run_);
	}

	void exit_process(int32_t ec)
	{
		if (on_exit_)
			std::invoke(on_exit_, pid_, ec);
	}

private:

	int32_t pid_ = 0;
	process_event_t on_exit_{nullptr};
	process_entry_t on_run_{nullptr};

};