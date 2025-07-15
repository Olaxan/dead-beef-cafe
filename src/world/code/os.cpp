#include "os.h"
#include "host.h"

#include <sstream>
#include <string>
#include <coroutine>
#include <chrono>

void OS::exec(std::string cmd)
{
	std::string temp{};
	std::vector<std::string> args;
	
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
        std::invoke(prog, *this, args);
		//int32_t pid = create_process();
    }
}

void OS::run()
{
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> sec(last_update_ - t1);
    last_update_ = t1;
    auto it = std::begin(task_queue_);
    while (it != std::end(task_queue_))
    {
        auto& [time, fn] = *it;
        if (time -= sec.count(); time < 0.f)
        {
            std::invoke(fn);
            it = task_queue_.erase(it);
        }
        else ++it;
    }
}

void OS::shutdown_os()
{
    owner_.shutdown_host();
}
