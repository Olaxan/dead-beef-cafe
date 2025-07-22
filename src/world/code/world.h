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

    template<typename T = OS>
    Host* create_host(std::string hostname)
    {
        auto ptr = std::make_unique<Host>(*this, hostname);
        Host& skel = *ptr;

        Disk& my_disk = skel.create_device<Disk>(500);
        CPU& my_cpu = skel.create_device<CPU>(1.5f);
        NIC& my_nic = skel.create_device<NIC>(100.f);
        FileSystem& fs = my_disk.create_fs();

        fs.add_file("welcome.txt");
        fs.add_file("boot.os", "...", FileModeFlags::Execute);

        skel.create_os<T>();

        hosts_.push_back(ptr);
        return hosts_.back().get();
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