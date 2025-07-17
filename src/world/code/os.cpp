#include "os.h"
#include "host.h"
#include "world.h"

#include <sstream>
#include <string>
#include <coroutine>
#include <chrono>
#include <iostream>
#include <print>

void OS::exec(std::string cmd)
{
	std::string temp{};
	std::vector<std::string> args{};
	
	std::stringstream ss(cmd);

	while (ss >> std::quoted(temp))
	{
		args.push_back(std::move(temp));
	}

    if (args.empty())
        return;

    std::string& word = *std::begin(args);

    if (auto it = commands_.find(word); it != commands_.end())
    {
        auto& prog = it->second;
		create_process(prog, args);
    }
}

void OS::shutdown_os()
{
    owner_.shutdown_host();
}

std::string OS::get_hostname() const
{
	return owner_.get_hostname();
}

World& OS::get_world()
{
    return owner_.get_world();
}

EagerTask<int32_t> OS::create_process(process_args_t program, std::vector<std::string> args)
{
    int32_t pid = pid_counter_++;
    std::println("Run {0} on {1} (pid = {2}).", args[0], (int64_t)this, pid);
    auto [it, success] = processes_.emplace(pid, std::invoke(program, this, std::move(args)));

    if (!success)
        co_return 1;

    it->second.handle.resume();
    int32_t ret = co_await it->second;
    
    std::println("Task {0} finished with code {1}.", pid, ret);
    processes_.erase(pid); // Use pid instead of iterator in case it's been invalidated after awaiting.
    co_return ret;
}

void OS::list_processes() const
{
    std::println("Processes on {0}:", get_hostname());

    for (auto& [pid, proc] : processes_)
        std::println("pid {0}: done='{1}', ready='{2}'", pid, proc.is_done(), proc.is_ready());
}

TimerAwaiter OS::wait(float seconds)
{
	return get_world().get_timer_manager().wait(seconds);
}

void OS::schedule(float seconds, schedule_fn callback)
{
    get_world().get_timer_manager().set_timer(seconds, callback);
}
