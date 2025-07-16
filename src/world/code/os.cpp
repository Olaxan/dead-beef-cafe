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
        std::println("Run {0} on {1}.", cmd, (int64_t)this);
		int32_t pid = create_process(prog, args);
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

void OS::list_processes() const
{
    std::println("Processes on {0}:", get_hostname());

    for (auto& [pid, proc] : processes_)
        std::println("pid {0}: ready='{1}'", pid, proc.is_ready());
}

TimerAwaiter OS::wait(float seconds)
{
	return get_world().get_timer_manager().wait(seconds);
}

void OS::schedule(float seconds, schedule_fn callback)
{
    get_world().get_timer_manager().set_timer(seconds, callback);
}
