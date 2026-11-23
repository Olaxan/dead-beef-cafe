#pragma once

#include "rel_mgr.h"
#include "uid64.h"

#include "host.h"
#include "person.h"
#include "person_types.h"
#include "business.h"
#include "business_types.h"
#include "internet.h"

#include <memory>
#include <vector>
#include <thread>
#include <random>

struct WorldData
{
	std::unordered_map<PersonId, Person> people;
    std::unordered_map<BusinessId, Business> corps;
    std::unordered_map<Uid64, std::unique_ptr<Host>> hosts;

	RelationManager<RelationshipType::OneToOne, PersonId, BusinessId> employment;
    RelationManager<RelationshipType::OneToMany, PersonId, Uid64> ownership;
    std::shared_ptr<InternetLinks> links{std::make_shared<InternetLinks>()};
};