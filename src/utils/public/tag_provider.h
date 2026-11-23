#pragma once

#include <optional>
#include <vector>
#include <string>

enum class RequireTagMode
{
	All = 0,
	AnyOrdered
};

template<typename T, typename TagT = std::string>
class ITagProvider
{
public:

	virtual std::optional<T> get_by_tag(TagT tag) const = 0;
	virtual std::optional<T> get_by_tags(std::vector<TagT> tags, RequireTagMode mode = RequireTagMode::All) const = 0;
	
};

using StringTagProvider = ITagProvider<std::string>;