#include "filesystem.h"

#include <ranges>
#include <algorithm>
#include <functional>
#include <print>

FileSystem::FileSystem()
{
	create_directory("root", 0);
}

bool FileSystem::is_directory_root(uint64_t fid) const
{
	if (auto it = roots_.find(fid); it != roots_.end())
		return it->second == fid; // If the root is itself, it counts as a root.
	
	return true;
}

std::string FileSystem::get_filename(uint64_t fid) const
{
	if (auto it = fid_to_name_.find(fid); it != fid_to_name_.end())
		return it->second;

	return "";
}

uint64_t FileSystem::get_fid(std::string filename) const
{
	if (auto it = name_to_fid_.find(filename); it != name_to_fid_.end())
		return it->second;

	return get_root();
}

std::vector<uint64_t> FileSystem::get_files(uint64_t dir) const
{
	if (!is_dir(dir))
		return {dir};

	std::vector<uint64_t> v{};

	auto range = mappings_.equal_range(dir);
	std::transform(range.first, range.second, std::back_inserter(v), [](std::pair<uint64_t, uint64_t> element){ return element.second; });

	return v;
}

uint64_t FileSystem::get_parent_folder(uint64_t fid) const
{
	if (auto it = roots_.find(fid); it != roots_.end())
		return it->second;

	return fid;
}

std::vector<uint64_t> FileSystem::get_root_chain(uint64_t fid) const
{
	if (!is_file(fid))
		return {};

	std::vector<uint64_t> chain = { fid };
	uint64_t current = fid;

	while (true)
	{
		current  = get_parent_folder(current);

		if (current == fid)
			break;

		chain.emplace_back(current);

		if (is_directory_root(current))
			break;
	}

	return chain;
}

uint64_t FileSystem::create_file(std::string filename, uint64_t root)
{
	return add_file<File>(filename, root);
}

uint64_t FileSystem::create_directory(std::string filename, uint64_t root)
{
	return add_file<File>(filename, root, FileModeFlags::Directory);
}
