#pragma once

#include "timer_mgr.h"
#include "game_srv.h"
#include "msg_queue.h"
#include "link_srv.h"
#include "rel_mgr.h"
#include "uid64.h"
#include "host.h"

#include "person.h"
#include "corp.h"

#include "proto/world.pb.h"

#include <memory>
#include <vector>
#include <thread>
#include <random>

using MessageFn = std::function<void(void)>;
using WorldUpdateQueue = MessageQueue<MessageFn>;

class World
{
public:

    World() = default;
    World(World&) = delete;

public:

    void init_world();
    void update_world(float delta_seconds);

    void launch();

    LinkServer& get_link_server() { return net_; }
    WorldUpdateQueue& get_update_queue() { return queue_; }
    TimerManager& get_timer_manager() { return timers_; }
    std::mt19937& get_random_stream() { return randomness_; }

    Host* add_host(Uid64 id, std::unique_ptr<Host>&& new_host);

    bool serialize(world::World* to);
    bool deserialize(const world::World* from);

private:

    const float min_timestep{0.01f};

    bool run_{true};
    std::jthread worker_{};
    std::chrono::steady_clock::time_point last_update_{};
    std::mt19937 randomness_;

    TimerManager timers_{};
    LinkServer net_{};
    WorldUpdateQueue queue_{};

public:

    GameServices services_
    {
        .timers = timers_
    };

    struct WorldData
    {
        std::unordered_map<PersonId, Person> people;
        std::unordered_map<CorpId, Corp> corps;
        std::unordered_map<Uid64, std::unique_ptr<Host>> hosts;
    } data;

    struct WorldRelations
    {
        RelationManager<RelationshipType::OneToOne, PersonId, CorpId> employment;
        RelationManager<RelationshipType::OneToMany, PersonId, Uid64> ownership;
    } relations;


};