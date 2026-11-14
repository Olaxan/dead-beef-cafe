#include "world.h"

#include "host.h"
#include "task.h"
#include "proc.h"

#include "audio_base.h"

#include "world_data.h"

#include <chrono>
#include <cmath>
#include <thread>

World::World(WorldExts&& exts)
: exts_(std::move(exts))
{ }

void World::create_world()
{
	data_ = std::make_shared<WorldData>();
}

void World::init_world()
{
	if (exts_.audio_impl)
	{
		exts_.audio_impl->init();
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

void World::deinit_world()
{
	if (exts_.audio_impl)
	{
		exts_.audio_impl->deinit();
	}
}

void World::launch()
{

	for (auto&& [uid, host] : data_->hosts)
	{
		host->setup(services_, create_host_context(uid));
		host->start_host();
	}

	worker_ = std::jthread([this]
	{
		while (run_.load())
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

void World::stop()
{
	run_.store(false);
	worker_.join();
}

Host* World::add_host(Uid64 id, std::unique_ptr<Host>&& new_host)
{
	auto [it, success] = data_->hosts.emplace(id, std::move(new_host));
	return (success) ? it->second.get() : nullptr;
}

bool World::serialize(world::World* to)
{
	for (auto& [id, host] : data_->hosts)
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
		if (auto it = data_->hosts.find(id); it != data_->hosts.end())
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

HostContext World::create_host_context()
{
	return create_host_context(Uid64Generator::generate());
}

HostContext World::create_host_context(Uid64 uid)
{
	return HostContext{
		.uid = uid,
		.internet = &internet_
	};
}

void World::import_data(std::shared_ptr<WorldData> new_data)
{
	stop();
	std::println("Deinitializing world...");
	std::println("Setting up world...");
	data_ = new_data;
	launch();
}
