#pragma once

#include <memory>
#include <vector>

#include "host.h"

class World
{
public:

    World() = default;

public:

    void update_world(float delta_seconds);

    template<typename T = Host>
    Host& create_host(std::string hostname)
    {
        return *hosts_.emplace_back(Host::create_skeleton_host(*this, hostname));
    }

private:

    std::vector<std::unique_ptr<Host>> hosts_{};

};