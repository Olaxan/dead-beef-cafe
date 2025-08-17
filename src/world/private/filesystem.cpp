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

void FilePath::substitute(std::string_view from, std::string_view to)
{
	if (from.empty())
		return;

	path_ = (path_ | std::views::split(from) | std::views::join_with(to) | std::ranges::to<std::string>());
	make();
}

void FilePath::make_absolute(std::string_view from_dir)
{
	if (from_dir.length() > 0)
	{
		path_ = std::format("{}/{}", from_dir, path_);
	}

	if (is_relative())
		path_.insert(0, "/");

	make();
}

void FilePath::make_absolute(const FilePath& from_dir)
{
	make_absolute(from_dir.get_view());
}


/* --- File System --- */

FileSystem::FileSystem()
{ 
	metadata_[get_root()] = {
		.is_directory = true,
		.owner_uid = 0,
		.owner_gid = 0,
		.perm_owner = FilePermissionTriad::All,
		.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
		.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute,
	};
}

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

	if (auto it = metadata_.find(fid); it != metadata_.end())
		return it->second.is_directory;

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
	std::string out(10, '-');

	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		const FilePermissionTriad& o = it->second.perm_owner;
		const FilePermissionTriad& g = it->second.perm_group;
		const FilePermissionTriad& u = it->second.perm_users;
		out[0] = is_dir(fid) ? 'd' : '-';

		out[1] = has_flag(o, FilePermissionTriad::Read) 	? 'r' : '-';
		out[2] = has_flag(o, FilePermissionTriad::Write) 	? 'w' : '-';
		out[3] = has_flag(o, FilePermissionTriad::Execute) 	? 'x' : '-';

		out[4] = has_flag(g, FilePermissionTriad::Read) 	? 'r' : '-';
		out[5] = has_flag(g, FilePermissionTriad::Write) 	? 'w' : '-';
		out[6] = has_flag(g, FilePermissionTriad::Execute) 	? 'x' : '-';

		out[7] = has_flag(u, FilePermissionTriad::Read) 	? 'r' : '-';
		out[8] = has_flag(u, FilePermissionTriad::Write) 	? 'w' : '-';
		out[9] = has_flag(u, FilePermissionTriad::Execute) 	? 'x' : '-';
	}

	return out;
}

std::pair<std::string, std::string> FileSystem::get_owner(uint64_t fid)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		return std::make_pair(
			std::format("{}", it->second.owner_uid), 
			std::format("{}", it->second.owner_gid));
	}
	return std::make_pair("-", "-");
}

std::string FileSystem::get_mdate(uint64_t fid)
{
	
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		std::chrono::system_clock::time_point tp{std::chrono::seconds{it->second.modified}};
		return std::format("{:%Y-%m-%d %X}", tp);
	}

	return "-";
}

uint64_t FileSystem::get_last_modified(uint64_t fid)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
		return it->second.modified;
		
	return 0;
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

FileOpResult FileSystem::create_file(const FilePath& path, const CreateFileParams& params)
{
	if (get_fid(path))
		return std::make_tuple(0, nullptr, FileSystemError::FileExists);

	if (params.recurse)
		create_ensure_path(path, params);

	return add_file<File>(path, params.meta);
}

FileOpResult FileSystem::create_directory(const FilePath& path, const CreateFileParams& params)
{
	FileOpResult res = create_file(path, params);
	if (auto [fid, ptr, err] = res; err == FileSystemError::Success)
	{
		file_set_directory_flag(fid, true);
	}
	return res;
}

uint64_t FileSystem::create_ensure_path(const FilePath& path, const CreateFileParams& params)
{
	FilePath parent = path.get_parent_path();
	if (uint64_t parent_fid = get_fid(parent))
	{
		return parent_fid;
	}
	else
	{
		auto [new_parent_fid, ptr, err] = create_directory(parent, params);
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

FileOpResult FileSystem::open(uint64_t fid, FileAccessFlags flags)
{
	if (is_dir(fid && has_flag<FileAccessFlags>(flags, FileAccessFlags::Write)))
		return std::make_tuple(fid, nullptr, FileSystemError::InvalidFlags);
	
	if (auto it = files_.find(fid); it != files_.end())
		return std::make_tuple(fid, it->second, FileSystemError::Success);

	return std::make_tuple(0, nullptr, FileSystemError::FileNotFound);
}

FileOpResult FileSystem::open(const FilePath& path, FileAccessFlags flags)
{
	if (uint64_t fid = get_fid(path); is_file(fid))
		return open(fid, flags);

	return std::make_tuple(0, nullptr, FileSystemError::FileNotFound);
}

void FileSystem::set_flag(FilePermissionTriad& base, FilePermissionTriad set_flags)
{
	base |= set_flags;
}

void FileSystem::clear_flag(FilePermissionTriad& base, FilePermissionTriad clear_flags)
{
	base = base & ~clear_flags;
}

bool FileSystem::has_flag(const FilePermissionTriad& base, FilePermissionTriad test_flags) const
{
	return (base & test_flags) == test_flags;
}

bool FileSystem::file_set_flag(uint64_t fid, FilePermissionCategory cat, FilePermissionTriad set_flags)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		file_set_flag(it->second, cat, set_flags);
		return true;
	}
	return false;
}

bool FileSystem::file_clear_flag(uint64_t fid, FilePermissionCategory cat, FilePermissionTriad clear_flags)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		file_clear_flag(it->second, cat, clear_flags);
		return true;
	}
	return false;
}

bool FileSystem::file_has_flag(uint64_t fid, FilePermissionCategory cat, FilePermissionTriad test_flags) const
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		file_has_flag(it->second, cat, test_flags);
		return true;
	}
	return false;
}

void FileSystem::file_set_flag(FileMeta& meta, FilePermissionCategory cat, FilePermissionTriad set_flags)
{
	switch (cat)
	{
		case FilePermissionCategory::Owner: set_flag(meta.perm_owner, set_flags); break;
		case FilePermissionCategory::Group: set_flag(meta.perm_group, set_flags); break;
		case FilePermissionCategory::Users: set_flag(meta.perm_users, set_flags); break;
	}
}

void FileSystem::file_clear_flag(FileMeta& meta, FilePermissionCategory cat, FilePermissionTriad clear_flags)
{
	switch (cat)
	{
		case FilePermissionCategory::Owner: clear_flag(meta.perm_owner, clear_flags); break;
		case FilePermissionCategory::Group: clear_flag(meta.perm_group, clear_flags); break;
		case FilePermissionCategory::Users: clear_flag(meta.perm_users, clear_flags); break;
	}
}

bool FileSystem::file_has_flag(const FileMeta& meta, FilePermissionCategory cat, FilePermissionTriad test_flags) const
{
	switch (cat)
	{
		case FilePermissionCategory::Owner: return has_flag(meta.perm_owner, test_flags);
		case FilePermissionCategory::Group: return has_flag(meta.perm_group, test_flags);
		case FilePermissionCategory::Users: return has_flag(meta.perm_users, test_flags);
	}
	return false;
}

bool FileSystem::file_set_directory_flag(uint64_t fid, bool new_is_dir)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		it->second.is_directory = new_is_dir;
		return true;
	}
	return false;
}

bool FileSystem::file_set_modified_now(uint64_t fid)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
		it->second.modified = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
		return true;
	}
	return false;
}

bool FileSystem::file_set_permissions(uint64_t fid, FilePermissionTriad owner, FilePermissionTriad group, FilePermissionTriad users)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		it->second.perm_owner = owner;
		it->second.perm_group = group;
		it->second.perm_users = users;
		return true;
	}
	return false;
}

bool FileSystem::file_set_permissions(uint64_t fid, int32_t owner, int32_t group, int32_t users)
{
	return file_set_permissions(fid, 
		static_cast<FilePermissionTriad>(owner),
		static_cast<FilePermissionTriad>(group),
		static_cast<FilePermissionTriad>(users));
}

bool FileSystem::check_permission(const SessionData& session, uint64_t fid, FileAccessFlags mode)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		FileMeta& meta = it->second;

		bool group_match = (session.gid == meta.owner_gid || session.groups.contains(meta.owner_gid));
		bool owner_match = (session.uid == meta.owner_uid);
		bool users_match = true;

		bool read_valid = std::invoke([&]() -> bool
		{
			if (!has_flag<FileAccessFlags>(mode, FileAccessFlags::Read))
				return true;

			if (file_has_flag(meta, FilePermissionCategory::Users, FilePermissionTriad::Read)) return true;
			if (file_has_flag(meta, FilePermissionCategory::Owner, FilePermissionTriad::Read) && owner_match) return true;
			if (file_has_flag(meta, FilePermissionCategory::Group, FilePermissionTriad::Read) && group_match) return true;

			return false;
		});

		bool write_valid = std::invoke([&]() -> bool
		{
			if (!has_flag<FileAccessFlags>(mode, FileAccessFlags::Write))
				return true;

			if (file_has_flag(meta, FilePermissionCategory::Users, FilePermissionTriad::Write)) return true;
			if (file_has_flag(meta, FilePermissionCategory::Owner, FilePermissionTriad::Write) && owner_match) return true;
			if (file_has_flag(meta, FilePermissionCategory::Group, FilePermissionTriad::Write) && group_match) return true;

			return false;
		});

		bool exec_valid = std::invoke([&]() -> bool
		{
			if (!has_flag<FileAccessFlags>(mode, FileAccessFlags::Execute))
				return true;

			if (file_has_flag(meta, FilePermissionCategory::Users, FilePermissionTriad::Execute)) return true;
			if (file_has_flag(meta, FilePermissionCategory::Owner, FilePermissionTriad::Execute) && owner_match) return true;
			if (file_has_flag(meta, FilePermissionCategory::Group, FilePermissionTriad::Execute) && group_match) return true;

			return false;
		});

		return read_valid && write_valid && exec_valid;
	}
	
	return false;
}
