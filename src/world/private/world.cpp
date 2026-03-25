#include "world.h"

#include "host.h"
#include "task.h"
#include "proc.h"

#include <chrono>
#include <cmath>
#include <thread>

void World::init_world()
{
	for (auto&& [id, host] : hosts_)
	{
		host->init(&services_);
	}
}

void World::update_world(float delta_seconds)
{
	/* Apply thread-safe updates. */
	if (auto opt = queue_.pop(); opt.has_value())
    {
        std::invoke(*opt);
    }

	/* Update timers. */
    timers_.step(delta_seconds);

}

void World::launch()
{
	init_world();
	
	worker_ = std::jthread([this]
	{
		while (run_)
		{
			auto t1 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> sec(t1 - last_update_);
			last_update_ = t1;

			float dt = sec.count();
			update_world(dt);

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	});
}

Host* World::add_host(Uid64 id, std::unique_ptr<Host>&& new_host)
{
	auto [it, success] = hosts_.emplace(id, std::move(new_host));
	return (success) ? it->second.get() : nullptr;
}

bool World::serialize(world::World* to)
{
	for (auto& [id, host] : hosts_)
	{
		world::Host* ar = to->add_hosts();
		ar->set_uid(id.num);
		host->serialize(ar);
	}

	return true;
}

bool World::deserialize(const world::World* from)
{
	for (auto&& ar : from->hosts())
	{
		Uid64 id = ar.uid();
		if (auto it = hosts_.find(id); it != hosts_.end())
		{
			Host* host = it->second.get();
			host->deserialize(ar);
		}
		else
		{
			// ... Create host
		}
	}

	return true;
}
