#pragma once

#include "timer_mgr.h"
#include "ip_mgr.h"
#include "msg_queue.h"

#include <memory>
#include <vector>
#include <thread>

using msg_func_t = std::function<void(void)>;
using WorldUpdateQueue = MessageQueue<msg_func_t>;

class Host;

class World
{
public:

    World() = default;

public:

    void update_world(float delta_seconds);

    void launch();

    WorldUpdateQueue& get_update_queue() { return queue_; }
    TimerManager& get_timer_manager() { return timers_; }
    IpManager& get_ip_manager() { return ip_; }

    Host* add_host(std::unique_ptr<Host>&& new_host);

private:

    const float min_timestep{0.01f};

    bool run_{true};
    std::jthread worker_{};
    std::chrono::steady_clock::time_point last_update_{};
    std::vector<std::unique_ptr<Host>> hosts_{};

    TimerManager timers_{};
    IpManager ip_{};

    WorldUpdateQueue queue_{};

};