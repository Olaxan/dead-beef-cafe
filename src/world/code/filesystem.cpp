#include "filesystem.h"

#include <ranges>
#include <algorithm>
#include <functional>
#include <print>

FileSystem::FileSystem()
{
	root_ = add_root_file<File>("root");
}

bool FileSystem::is_dir(uint64_t fid) const
{
	if (auto it = files_.find(fid); it != files_.end())
		return it->second->has_flag(FileModeFlags::Directory);

	return false;
}

bool FileSystem::is_directory_root(uint64_t fid) const
{
	if (auto it = roots_.find(fid); it != roots_.end())
		return it->second == fid; // If the root is itself, it counts as a root.
	
	return true;
}

bool FileSystem::is_empty(uint64_t fid) const
{
	if (!is_dir(fid))
		return true;

	std::vector<uint64_t> files = get_files(fid);
	return files.empty();
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

	return 0;
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

bool FileSystem::remove_file(uint64_t fid, bool recurse)
{
	if (is_empty(fid))
	{
		mappings_.erase(fid);
		roots_.erase(fid);
		if (auto it = fid_to_name_.find(fid); it != fid_to_name_.end())
		{
			name_to_fid_.erase(it->second);
			fid_to_name_.erase(it);
		}
		return true;
	}
	else if (recurse)
	{
		for (uint64_t child : get_files(fid))
		{
			if (!remove_file(child, true))
				return false;
		}
		return remove_file(fid, false);
	}

	return false;
}
