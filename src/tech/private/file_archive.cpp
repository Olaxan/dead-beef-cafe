#include "file_archive.h"

#include "rand_char.h"

#include <string_view>
#include <print>

FileArchive::FileArchive() = default;

void FileArchive::add_datasource(std::shared_ptr<IDatasourceBase> source)
{
	std::string name = source->get_name();
	sources_.emplace_back(std::move(source));
	std::println("Installed datapack '{}'.", name);
}

ArHandle FileArchive::add_file(std::string content, std::vector<std::string>&& tags)
{
	ArHandle h{ Uid64{0}, ++next_h_};
	auto hash = std::hash<std::string>()(content);

	pages_.try_emplace(h, content);
	hash_lookup_.try_emplace(hash, h);
	tag_lookup_.add_tag_value(h, std::forward<std::vector<std::string>>(tags));

	return h;
}

ArHandle FileArchive::add_program(ProcessFn&& func, std::vector<std::string>&& tags)
{
	std::string content = RandChars{}(1024);
	ArHandle h = add_file(content, std::forward<std::vector<std::string>>(tags));
	execs_.try_emplace(h, func);
	return h;
}

std::optional<ArHandle> FileArchive::get_by_tag(std::string tag) const
{
	return tag_lookup_.get_by_tag(tag);
}

std::optional<ArHandle> FileArchive::get_by_tags(std::vector<std::string> tags, RequireTagMode mode) const
{
	return tag_lookup_.get_by_tags(std::forward<std::vector<std::string>>(tags), mode);
}

std::optional<ArHandle> FileArchive::find_by_digest(std::string_view data) const
{
	auto hash = std::hash<std::string_view>()(data);

	if (auto it = hash_lookup_.find(hash); it != hash_lookup_.end())
		return it->second;

	return std::nullopt;
}

std::optional<std::string_view> FileArchive::try_get_bytes(ArHandle h) const
{
	if (auto it = pages_.find(h); it != pages_.end())
		return std::string_view{it->second.bytes};

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
		return it->second;

	return std::nullopt;
}

std::vector<std::string> FileArchive::get_all_program_tags() const
{
	std::vector<std::string> out;
	for (auto&& [ar, fn] : execs_)
		out.append_range(tag_lookup_.get_tags_for(ar));

	return out;
}

std::vector<ArHandle> FileArchive::get_all_programs() const
{
	std::vector<ArHandle> out;
	for (auto&& [ar, fn] : execs_)
		out.emplace_back(ar);

	return out;
}
