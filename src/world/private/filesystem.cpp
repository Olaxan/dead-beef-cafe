#include "filesystem.h"

#include <ranges>
#include <algorithm>
#include <functional>
#include <print>
#include <iostream>

/* --- File Path --- */

FilePath::FilePath(std::string path)
{
	path_ = path.empty() ? "/" : path;
	make();
}

void FilePath::make()
{
	valid_ = false;

	if (path_.empty())
		return;

	relative_ = (path_[0] != '/');

	if (path_.back() == '/')
		path_.pop_back();

	valid_ = true;
}

bool FilePath::is_backup() const
{
	std::string_view name = get_name();
	return name.back() == '~';
}

bool FilePath::is_config() const
{
	std::string_view name = get_name();
	return name.front() == '.';
}

std::string_view FilePath::get_parent_view() const
{
	std::string_view path(path_);
	if (!path.empty() && path.back() == '/')
        path.remove_suffix(1);

    // Find last slash
    auto last_slash = path.rfind('/');
    if (last_slash == std::string_view::npos)
        return {}; // No parent

    return path.substr(0, last_slash);
}

FilePath FilePath::get_parent_path() const
{
	return FilePath(get_parent_view());
}

std::string_view FilePath::get_name() const
{
	std::string_view path(path_);

	if (!path.empty() && path.back() == '/')
        path.remove_suffix(1);

    // Find last slash
    auto last_slash = path.rfind('/');
    if (last_slash == std::string_view::npos)
        return {}; // No parent

    return path.substr(last_slash + 1);
}

void FilePath::prepend(const FilePath& other)
{
	if (!other.is_valid_path())
		return;

	const std::string& other_path = other.path_;
	path_ = std::format("{}/{}", other_path, path_);
	make();
}

void FilePath::append(const FilePath& other)
{
	if (!other.is_valid_path())
		return;

	const std::string& other_path = other.path_;
	path_ = std::format("{}/{}", path_, other_path);
	make();
}


/* --- File System --- */

FileSystem::FileSystem()
{ }

const char* FileSystem::get_fserror_name(FileSystemError code)
{
	switch (code)
	{
		case FileSystemError::FileNotFound: return "File not found";
		case FileSystemError::FileExists: return "The file already exists";
		case FileSystemError::FolderNotEmpty: return "Folder not empty";
		case FileSystemError::InsufficientPermissions: return "Insufficient permissions";
		case FileSystemError::InvalidFlags: return "Invalid file flags";
		case FileSystemError::IOError: return "I/O error";
		case FileSystemError::PreserveRoot: return "The operation can't be performed on the root directory";
		case FileSystemError::Other: return "Unknown error";
		case FileSystemError::Success: return "The operation completed successfully";
		default: return "Unknown error";
	}
}

bool FileSystem::is_file(uint64_t fid) const
{
	return fid == get_root() || files_.contains(fid);
}

bool FileSystem::is_file(const FilePath& path) const
{
	return get_fid(path) != 0;
}

bool FileSystem::is_dir(uint64_t fid) const
{
	if (fid == get_root())
		return true;

	if (auto it = files_.find(fid); it != files_.end())
		return it->second->has_flag(FileModeFlags::Directory);

	return false;
}

bool FileSystem::is_dir(const FilePath& path) const
{
	return is_dir(get_fid(path));
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

std::string_view FileSystem::get_filename(uint64_t fid) const
{
	if (auto it = fid_to_path_.find(fid); it != fid_to_path_.end())
		return it->second.get_name();

	return {};
}

FilePath FileSystem::get_path(uint64_t fid) const
{
	if (fid == get_root())
		return {"/"};

	if (auto it = fid_to_path_.find(fid); it != fid_to_path_.end())
		return it->second;

	return {};
}

uint64_t FileSystem::get_fid(const FilePath& path) const
{
	if (path.is_root_or_empty())
		return get_root();

	if (auto it = path_to_fid_.find(path); it != path_to_fid_.end())
		return it->second;

	return 0;
}

std::size_t FileSystem::get_links(uint64_t fid)
{
	return mappings_.count(fid);
}

std::size_t FileSystem::get_bytes(uint64_t fid)
{
	if (auto it = files_.find(fid); it != files_.end())
	{
		return it->second->size();
	}

	return 0;
}

std::string FileSystem::get_flags(uint64_t fid)
{
	std::string out(5, '-');

	if (auto it = files_.find(fid); it != files_.end())
	{
		const File& file = *it->second;
		out[0] = file.has_flag(FileModeFlags::Directory) 	? 'd' : '-';
		out[1] = file.has_flag(FileModeFlags::System)		? 's' : '-';
		out[2] = file.has_flag(FileModeFlags::Read) 		? 'r' : '-';
		out[3] = file.has_flag(FileModeFlags::Write) 		? 'w' : '-';
		out[4] = file.has_flag(FileModeFlags::Execute) 		? 'x' : '-';
		/* This will need modifying when file permissions get extended. */
	}

	return out;
}

std::string FileSystem::get_owner(uint64_t fid)
{
	return "system"; //not implemented
}

std::string FileSystem::get_mdate(uint64_t fid)
{
	return "2025-08-23 16:32"; //not implemented
}

std::vector<uint64_t> FileSystem::get_files(uint64_t dir, bool recurse) const
{
	if (!is_dir(dir))
		return {};

	std::vector<uint64_t> v{};

	auto range = mappings_.equal_range(dir);
	std::transform(range.first, range.second, std::back_inserter(v), [](std::pair<uint64_t, uint64_t> element){ return element.second; });

	if (recurse)
	{
		std::vector<uint64_t> v2{};
		for (uint64_t child : v)
		{
			v2.append_range(get_files(child, true));
		}
		v.append_range(v2);
	}

	return v;
}

std::vector<uint64_t> FileSystem::get_files(const FilePath& path, bool recurse) const
{
	return get_files(get_fid(path), recurse);
}

std::vector<FilePath> FileSystem::get_paths(uint64_t fid, bool recurse) const
{
	std::vector<FilePath> out;
	std::ranges::transform(get_files(fid, recurse), std::back_inserter(out), [this](uint64_t node){ return get_path(node); });
	return out;
}

std::vector<FilePath> FileSystem::get_paths(const FilePath& path, bool recurse) const
{
	return get_paths(get_fid(path), recurse);
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

FileOpResult FileSystem::create_file(const FilePath& path, bool recurse)
{
	if (get_fid(path))
		return std::make_tuple(0, nullptr, FileSystemError::FileExists);

	//std::println("Creating file under '{}' (recurse = {}).", path, recurse);
	
	if (recurse)
		create_ensure_path(path);

	return add_file<File>(path);
}

FileOpResult FileSystem::create_directory(const FilePath& path, bool recurse)
{
	if (get_fid(path))
		return std::make_tuple(0, nullptr, FileSystemError::FileExists);
	
	//std::println("Creating dir under '{}' (recurse = {}).", path, recurse);

	if (recurse)
		create_ensure_path(path);

	return add_file<File>(path, FileModeFlags::Directory);
}

uint64_t FileSystem::create_ensure_path(const FilePath& path)
{
	FilePath parent = path.get_parent_path();
	if (uint64_t parent_fid = get_fid(parent))
	{
		//std::println("Directory parent '{}' ({}) exists.", parent, parent_fid);
		return parent_fid;
	}
	else
	{
		auto [new_parent_fid, ptr, err] = create_directory(parent, true);
		//std::println("Directory parent '{}' ({}) was created.", parent, new_parent_fid);
		return new_parent_fid;
	}
}

bool FileSystem::remove_file(uint64_t fid, FileRemoverFn&& func)
{
	FilePath path = get_path(fid);
	return remove_file(path, std::forward<FileRemoverFn>(func));
}

bool FileSystem::remove_file(const FilePath& path, FileRemoverFn&& func)
{
	uint64_t fid = get_fid(path);

	/* If this is the root directory, ensure we can operate on it. */
	if (path == "/" && !func(*this, path, FileSystemError::PreserveRoot))
	{
		return false;
	}

	/* If this file doesn't exist, report it to callback and return. */
	if (!is_file(fid))
	{
		func(*this, path, FileSystemError::FileNotFound);
		return false;
	}

	/* If we're not empty, and the callback doesn't say that's okay, fail. */
	if (!(is_empty(fid) || func(*this, path, FileSystemError::FolderNotEmpty)))
	{
		return false;
	}

	/* If we get here, the callback must have given green light for recursion.
	Remove all children. */
	for (uint64_t child : get_files(fid))
	{
		if (!remove_file(child, std::forward<FileRemoverFn>(func)))
		{
			return false;
		}
	}

	/* Even if callback requests resume, fail if not empty after recursion. */
	if (!is_empty(fid))
	{
		func(*this, path, FileSystemError::FolderNotEmpty);
		return false;
	}

	/* Get the root of this file and erase the file from its parent mapping,
	if such a root exists. */
	if (auto parent = roots_.find(fid); parent != roots_.end())
	{
		auto range = mappings_.equal_range(parent->second);
		auto it = range.first;
		
		for (; it != range.second; )
		{
			if (it->second == fid)
			{
				it = mappings_.erase(it);
			}
			else ++it;
		}

		/* Actually remove this file from the roots map. */
		roots_.erase(parent);
	}

	/* Remove the file from the path-fid/fid-path mappings. */
	if (auto it = fid_to_path_.find(fid); it != fid_to_path_.end())
	{
		path_to_fid_.erase(it->second);
		fid_to_path_.erase(it);
	}

	/* Remove the file itself. */
	files_.erase(fid);

	return func(*this, path, FileSystemError::Success);

}

FileSystemError FileSystem::remove_file(uint64_t fid, bool recurse)
{
	if (!is_file(fid))
		return FileSystemError::FileNotFound;

	if (is_empty(fid))
	{
		/* Get the root of this file and erase the file from its parent mapping,
		if such a root exists. */
		if (auto parent = roots_.find(fid); parent != roots_.end())
		{
			auto range = mappings_.equal_range(parent->second);
			auto it = range.first;
			
			for (; it != range.second; )
			{
				if (it->second == fid)
				{
					it = mappings_.erase(it);
				}
				else ++it;
			}

			/* Actually remove this file from the roots map. */
			roots_.erase(parent);
		}

		/* Remove the file from the path-fid/fid-path mappings. */
		if (auto it = fid_to_path_.find(fid); it != fid_to_path_.end())
		{
			path_to_fid_.erase(it->second);
			fid_to_path_.erase(it);
		}

		/* Remove the file itself. */
		files_.erase(fid);

		return FileSystemError::Success;
	}
	else if (recurse)
	{
		for (uint64_t child : get_files(fid))
		{
			if (FileSystemError err = remove_file(child, true); err != FileSystemError::Success)
				return err;
		}
		return remove_file(fid, false);
	}

	return FileSystemError::FolderNotEmpty;
}

FileSystemError FileSystem::remove_file(const FilePath& path, bool recurse)
{
	uint64_t fid = get_fid(path);
	return remove_file(fid, recurse);
}

FileOpResult FileSystem::open(const FilePath& path, FileAccessFlags flags)
{
	if (uint64_t fid = get_fid(path); is_file(fid))
	{
		if (is_dir(fid))
			return std::make_tuple(fid, nullptr, FileSystemError::InvalidFlags);
		
		if (auto it = files_.find(fid); it != files_.end())
			return std::make_tuple(fid, it->second.get(), FileSystemError::Success);
	}

	/* Not equal here! */
	if (flags == FileAccessFlags::Create)
	{
		auto [new_fid, ptr, err] = create_file(path);
		if (err == FileSystemError::Success)
			return open(path, flags);
	}

	return std::make_tuple(0, nullptr, FileSystemError::FileNotFound);
}
