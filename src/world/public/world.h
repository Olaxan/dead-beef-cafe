#pragma once

#include "timer_mgr.h"
#include "net_srv.h"
#include "msg_queue.h"
#include "link_srv.h"
#include "host.h"
#include "uuid.h"

#include "proto/world.pb.h"

#include <memory>
#include <vector>
#include <thread>

using MessageFn = std::function<void(void)>;
using WorldUpdateQueue = MessageQueue<MessageFn>;

class World
{
public:

    World() = default;
    World(World&) = delete;

public:

    void update_world(float delta_seconds);

    void launch();

    LinkServer& get_link_server() { return net_; }
    WorldUpdateQueue& get_update_queue() { return queue_; }
    TimerManager& get_timer_manager() { return timers_; }
    IpManager& get_ip_manager() { return ip_; }

    Host* add_host(Uid64 id, std::unique_ptr<Host>&& new_host);

    bool serialize(world::World* to);
    bool deserialize(const world::World* from);

private:

    const float min_timestep{0.01f};

    bool run_{true};
    std::jthread worker_{};
    std::chrono::steady_clock::time_point last_update_{};
    std::unordered_map<Uid64, std::unique_ptr<Host>> hosts_{};

    TimerManager timers_{};
    IpManager ip_{};

    WorldUpdateQueue queue_{};

    LinkServer net_{};

};