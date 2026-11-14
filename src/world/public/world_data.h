#pragma once

#include "rel_mgr.h"
#include "uid64.h"

#include "host.h"
#include "person.h"
#include "corp.h"

#include <memory>
#include <vector>
#include <thread>
#include <random>

struct WorldData
{
	std::unordered_map<PersonId, Person> people;
    std::unordered_map<CorpId, Corp> corps;
    std::unordered_map<Uid64, std::unique_ptr<Host>> hosts;

	RelationManager<RelationshipType::OneToOne, PersonId, CorpId> employment;
    RelationManager<RelationshipType::OneToMany, PersonId, Uid64> ownership;
    RelationManager<RelationshipType::OneToMany, Uid64, Uid64> links;
};