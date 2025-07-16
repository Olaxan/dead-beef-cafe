#include "world.h"

#include <chrono>
#include <cmath>
#include <thread>

void World::update_world(float delta_seconds)
{
	if (auto opt = queue_.pop(); opt.has_value())
    {
        std::invoke(*opt);
    }

    timers_.step(delta_seconds);
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