#pragma once

#include <cstdint>
#include <unordered_set>

struct SessionData
{
	SessionData() = default;
	SessionData(int32_t sid, int32_t uid, int32_t gid)
		: sid(sid), uid(uid), gid(gid) {}

	int32_t gid{-1};
	int32_t sid{-1};
	int32_t uid{-1};

	std::unordered_set<int32_t> groups;
};