#pragma once

#include "uid64.h"

#include <string>
#include <cstdint>
#include <compare>

struct ArHandle
{
	ArHandle() = default;

	Uid64 archive_id{0};
	int64_t handle{-1};
	auto operator <=> (const ArHandle&) const = default;

	bool is_valid() const
	{
		return handle != -1;
	}
};

template <>
struct std::hash<ArHandle>
{
  	std::size_t operator()(const ArHandle& k) const
	{
    	return std::hash<Uid64>()(k.archive_id) ^ std::hash<uint64_t>()(k.handle);
	}
};

struct ArPage
{
	std::string bytes;
};