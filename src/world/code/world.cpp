#include "world.h"

#include "host.h"
#include "task.h"
#include "proc.h"

#include <chrono>
#include <cmath>
#include <thread>

void World::update_world(float delta_seconds)
{
	/* Apply thread-safe updates. */
	if (auto opt = queue_.pop(); opt.has_value())
    {
        std::invoke(*opt);
    }

	/* Update timers. */
    timers_.step(delta_seconds);

	/* Update routing. */
	ip_.step(delta_seconds);
}

void World::launch()
{
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

Host* World::add_host(std::unique_ptr<Host>&& new_host)
{
	return hosts_.emplace_back(std::move(new_host)).get();
}
