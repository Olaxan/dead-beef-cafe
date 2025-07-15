#include "os.h"
#include "host.h"

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

void OS::run()
{
    if (auto opt = msg_queue_.pop(); opt.has_value())
    {
        exec(*opt);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> sec(t1 - last_update_);
    last_update_ = t1;

    std::vector<schedule_fn> callbacks{};
    for (auto it = task_queue_.begin(); it != task_queue_.end();)
    {
        auto& [time, fn] = *it;

        if (time -= sec.count(); time < 0.f)
        {
            callbacks.push_back(std::move(fn));
            it = task_queue_.erase(it);
        }
        else ++it;
    }

    for (auto& cb : callbacks)
        std::invoke(cb);
}

void OS::shutdown_os()
{
    owner_.shutdown_host();
}

std::string OS::get_hostname() const
{
	return owner_.get_hostname();
}
