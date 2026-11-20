#pragma once

#include <optional>
#include <vector>
#include <string>

template<typename T, typename TagT = std::string>
class ITagProvider
{
public:

	virtual std::optional<T> get_by_tag(TagT tag) = 0;
	virtual std::optional<T> get_by_tags(std::vector<TagT> tags) = 0;
	
};