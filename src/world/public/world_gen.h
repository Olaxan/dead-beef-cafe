#pragma once

#include "world.h"
#include "person.h"
#include "corp.h"

#include <random>

namespace WorldGen
{
	PersonId generate_person(World& world);
	CorpId generate_corp(World& world);
};