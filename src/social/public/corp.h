#pragma once

#include <cstdint>
#include <string>

struct CorpId
{
	int32_t id{-1};
	auto operator <=> (const CorpId&) const = default;
};

template <>
struct std::hash<CorpId>
{
  	std::size_t operator()(const CorpId& k) const
	{
    	return std::hash<int32_t>()(k.id);
	}
};

struct Corp
{
	std::string name{"Business"};
};