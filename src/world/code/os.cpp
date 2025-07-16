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
        std::invoke(prog, this, args);
		//int32_t pid = create_process();
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

AsyncTimeAwaiter OS::wait(float seconds)
{
	return get_world().get_timer_manager().wait(seconds);
}

void OS::schedule(float seconds, schedule_fn callback)
{
    get_world().get_timer_manager().set_timer(seconds, callback);
}
