#pragma once

#include "uid64.h"
#include "proc_types.h"
#include "tag_mgr.h"
#include "tag_provider.h"

#include <variant>

struct ArHandle
{
	Uid64 archive_id{0};
	int64_t handle{-1};
	auto operator <=> (const ArHandle&) const = default;
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

class FileArchive
{
public:

	FileArchive();

	std::optional<ArHandle> find_by_digest(std::string_view data) const;

	std::optional<ProcessFn> try_get_exec(std::string_view data) const;
	std::optional<ProcessFn> try_get_exec(ArHandle handle) const;

protected:

protected:

	std::vector<ArPage> pages_;

	TagManager<ArHandle> tag_lookup_;
	std::unordered_map<std::size_t, ArHandle> hash_lookup_;
	std::unordered_map<ArHandle, ProcessFn> execs_;

};