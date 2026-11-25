#pragma once

#include "tag_provider.h"

#include <vector>
#include <optional>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <variant>
#include <utility>

template<typename T, typename TagT = std::string>
class TagManager : public ITagProvider<T, TagT>
{
public:

	void add_tag_value(T val, TagT tag);
	void add_tag_value(T val, std::vector<TagT> tags);
	void add_tag_value(std::function<T()> source, TagT tag);
	void add_tag_value(std::function<T()> source, std::vector<TagT> tags);

	std::optional<T> get_by_tag(TagT tag) const override;
	std::optional<T> get_by_tags(std::vector<TagT> tags, RequireTagMode mode = RequireTagMode::All) const override;

protected:

	std::optional<std::size_t> get_index_by_tag(TagT tag) const;
	std::optional<T> get_by_index(std::size_t index) const;

protected:

	std::vector<std::variant<T, std::function<T()>>> data_;
	std::unordered_map<TagT, std::size_t> lookup_;

};

template<typename T, typename TagT>
inline void TagManager<T, TagT>::add_tag_value(T val, TagT tag)
{
	std::size_t index = data_.size();
	data_.emplace_back(std::in_place_type<T>, std::move(val));
	lookup_[tag] = index;
}

template<typename T, typename TagT>
inline void TagManager<T, TagT>::add_tag_value(T val, std::vector<TagT> tags)
{
	std::size_t index = data_.size();
	data_.emplace_back(std::in_place_type<T>, std::move(val));

	for (auto&& tag : tags)
	{
		lookup_[tag] = index;
	}
}

template<typename T, typename TagT>
inline void TagManager<T, TagT>::add_tag_value(std::function<T()> source, TagT tag)
{
	std::size_t index = data_.size();
	data_.emplace_back(std::in_place_type<std::function<T()>>, std::move(source));
	lookup_[tag] = index;
}

template<typename T, typename TagT>
inline void TagManager<T, TagT>::add_tag_value(std::function<T()> source, std::vector<TagT> tags)
{
	std::size_t index = data_.size();
	data_.emplace_back(std::in_place_type<std::function<T()>>, std::move(source));

	for (auto&& tag : tags)
	{
		lookup_[tag] = index;
	}
}

template<typename T, typename TagT>
inline std::optional<T> TagManager<T, TagT>::get_by_tag(TagT tag) const
{
	if (auto opt_idx = get_index_by_tag(tag); opt_idx.has_value())
	{
		const auto& entry = data_[*opt_idx];
		if (std::holds_alternative<T>(entry))
		{
			return std::get<T>(entry);
		}

		const auto& source = std::get<std::function<T()>>(entry);
		if (source)
		{
			return source();
		}
	}
	return std::nullopt;
}

template<typename T, typename TagT>
inline std::optional<T> TagManager<T, TagT>::get_by_tags(std::vector<TagT> tags, RequireTagMode mode) const
{
	std::unordered_map<std::size_t, std::size_t> histogram;

	for (auto&& tag : tags)
	{
		auto opt_idx = get_index_by_tag(tag);

		if (!opt_idx && mode == RequireTagMode::All)
		{
			return std::nullopt;
		}

		if (opt_idx)
		{
			++histogram[*opt_idx];
		}
	}

	if (histogram.empty())
	{
		return std::nullopt;
	}

	auto best = std::ranges::max_element(histogram, [](const auto& lhs, const auto& rhs)
	{
		return lhs.second < rhs.second;
	});

	return get_by_index(best->first);
}

template<typename T, typename TagT>
inline std::optional<std::size_t> TagManager<T, TagT>::get_index_by_tag(TagT tag) const
{
	if (auto it = lookup_.find(tag); it != lookup_.end())
	{
		return it->second;
	}
	return std::nullopt;
}

template<typename T, typename TagT>
inline std::optional<T> TagManager<T, TagT>::get_by_index(std::size_t index) const
{
	if (index >= data_.size())
	{
		return std::nullopt;
	}

	const auto& entry = data_[index];
	if (std::holds_alternative<T>(entry))
	{
		return std::get<T>(entry);
	}

	const auto& source = std::get<std::function<T()>>(entry);
	if (source)
	{
		return source();
	}

	return std::nullopt;
}
