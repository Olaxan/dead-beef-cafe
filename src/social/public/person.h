#pragma once

#include <cstdint>
#include <string>

struct PersonId
{
	int32_t id{-1};
	auto operator <=> (const PersonId&) const = default;
};

template <>
struct std::hash<PersonId>
{
  	std::size_t operator()(const PersonId& k) const
	{
    	return std::hash<int32_t>()(k.id);
	}
};

struct Person
{
	std::string forename{"John"};
	std::string surname{"Doe"};
	std::size_t salary{50000};
};
