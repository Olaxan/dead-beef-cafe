#pragma once

#include "timer_mgr.h"
#include "net_srv.h"
#include "msg_queue.h"
#include "link_srv.h"

#include <memory>
#include <vector>
#include <thread>

class Host;

using MessageFn = std::function<void(void)>;
using WorldUpdateQueue = MessageQueue<MessageFn>;
using HostLinkServer = LinkServer<Host*>;


class World
{
public:

    World() = default;
    World(World&) = delete;

public:

    void update_world(float delta_seconds);

    void launch();

    HostLinkServer& get_link_server() { return net_; }
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

    HostLinkServer net_{};

};