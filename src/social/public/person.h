#pragma once

#include "person_types.h"

#include <cstdint>
#include <string>

struct Person
{
	std::string forename{"John"};
	std::string surname{"Doe"};
	std::size_t salary{50000};
};
