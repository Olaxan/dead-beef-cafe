#pragma once

#include "timer_mgr.h"
#include "game_srv.h"
#include "msg_queue.h"
#include "link_srv.h"
#include "rel_mgr.h"
#include "uid64.h"
#include "host.h"
#include "host_context.h"
#include "internet.h"

#include "person.h"
#include "corp.h"

#include "proto/world.pb.h"

#include <memory>
#include <vector>
#include <thread>
#include <random>

using MessageFn = std::function<void(void)>;
using WorldUpdateQueue = MessageQueue<MessageFn>;

struct WorldExts
{
    IAudioBase* audio_impl;
};

class World
{
public:

    World(WorldExts&& exts);
    World(World&) = delete;

public:

    void init_world();
    void update_world(float delta_seconds);
    void deinit_world();

    void launch();

    LinkServer& get_link_server() { return net_; }
    WorldUpdateQueue& get_update_queue() { return queue_; }
    TimerManager& get_timer_manager() { return timers_; }
    Internet& get_internet() { return internet_; }

    Host* add_host(Uid64 id, std::unique_ptr<Host>&& new_host);

    bool serialize(world::World* to);
    bool deserialize(const world::World* from);

    GameServices& get_services() { return services_; }
    HostContext create_host_context();
    HostContext create_host_context(Uid64 uid);

private:

    /* World runner variables. */
    const float min_timestep{0.01f};

    bool run_{true};
    std::jthread worker_{};
    std::chrono::steady_clock::time_point last_update_{};

private:

    /* Core owned objects. */
    TimerManager timers_{};
    LinkServer net_{};
    WorldUpdateQueue queue_{};
    Internet internet_{};

    /* Implementation-specific dependencies (i.e. sound). */
    WorldExts exts_;

    /* Views and contexts into world-owned members. */
    GameServices services_
    {
        .outer = this,
        .timers = &timers_,
        .audio = exts_.audio_impl
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
        RelationManager<RelationshipType::OneToMany, Uid64, Uid64> links;
    } relations;
    
};