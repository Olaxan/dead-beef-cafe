#include "world_gen.h"

PersonId WorldGen::generate_person(World& world)
{
	return PersonId();
}

CorpId WorldGen::generate_corp(World& world)
{
	CorpId new_id{-1};
	Corp new_corp{"Business"}; // Pull from file
	
	PersonId admin = generate_person(world);
	world.relations.employment.relate(admin, new_id);

	for (std::size_t i = 0; i < 25; ++i)
	{
		PersonId employee = generate_person(world);
		world.relations.employment.relate(employee, new_id);
	}
}