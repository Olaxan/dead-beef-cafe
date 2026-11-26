#pragma once

#include "file_archive_types.h"

#include "uid64.h"
#include "proc_types.h"
#include "tag_mgr.h"
#include "tag_provider.h"

#include <variant>
#include <optional>
#include <unordered_map>
#include <string_view>


class FileArchive
{
public:

	FileArchive();

	std::optional<ArHandle> find_by_digest(std::string_view data) const;
	
	std::optional<std::string_view> try_get_bytes(ArHandle h) const;

	std::optional<ProcessFn> try_get_exec(std::string_view data) const;
	std::optional<ProcessFn> try_get_exec(ArHandle handle) const;

protected:

protected:

	TagManager<ArHandle> tag_lookup_;
	std::unordered_map<std::size_t, ArHandle> hash_lookup_;
	std::unordered_map<ArHandle, ProcessFn> execs_;
	std::unordered_map<ArHandle, ArPage> pages_;

};