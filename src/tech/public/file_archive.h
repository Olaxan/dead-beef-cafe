#pragma once

#include "proc_types.h"
#include "tag_mgr.h"
#include "tag_provider.h"

#include <variant>

struct ArHandle
{
	int32_t h{-1};
	auto operator <=> (const ArHandle&) const = default;
}

class FileArchive
{
public:

	FileArchive();

protected:

protected:

	TagManager<ArHandle> files_;

};