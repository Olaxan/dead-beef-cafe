#include "file_archive.h"

#include <string_view>

FileArchive::FileArchive() = default;

std::optional<ArHandle> FileArchive::find_by_digest(std::string_view data) const
{
	auto hash = std::hash<std::string_view>()(data);

	if (auto it = hash_lookup_.find(hash); it != hash_lookup_.end())
	{
		return it->second;
	}

	return std::nullopt;
}

std::optional<ProcessFn> FileArchive::try_get_exec(std::string_view data) const
{
	return find_by_digest(data).and_then([&](const ArHandle& h)
	{
		return try_get_exec(h);
	});
}

std::optional<ProcessFn> FileArchive::try_get_exec(ArHandle handle) const
{
	if (auto it = execs_.find(handle); it != execs_.end())
	{
		return it->second;
	}

	return std::nullopt;
}
