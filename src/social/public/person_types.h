#pragma once

#include <cstdint>
#include <compare>

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