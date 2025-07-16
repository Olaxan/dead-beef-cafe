#pragma once

#include <memory>
#include <vector>
#include <thread>

#include "host.h"
#include "timer_mgr.h"
#include "msg_queue.h"

using msg_func_t = std::function<void(void)>;
using WorldUpdateQueue = MessageQueue<msg_func_t>;

class World
{
public:

    World() = default;

public:

    void update_world(float delta_seconds);

    void launch();

    WorldUpdateQueue& get_update_queue() { return queue_; }
    TimerManager& get_timer_manager() { return timers_; }

    template<typename T = Host>
    Host& create_host(std::string hostname)
    {
        return *hosts_.emplace_back(Host::create_skeleton_host(*this, hostname));
    }

private:

    const float min_timestep{0.01f};

    bool run_{true};
    std::jthread worker_{};
    std::chrono::steady_clock::time_point last_update_{};
    std::vector<std::unique_ptr<Host>> hosts_{};
    TimerManager timers_{};
    WorldUpdateQueue queue_{};

};