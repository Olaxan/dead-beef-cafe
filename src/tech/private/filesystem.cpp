#include "filesystem.h"
#include "session.h"

#include <ranges>
#include <algorithm>
#include <functional>
#include <print>
#include <iostream>
#include <cassert>

#include "proto/files.pb.h"

#include <iso646.h>

/* --- File System --- */

FileSystem::FileSystem()
{ 
	metadata_[get_root()] = {
		.owner_uid = 0,
		.owner_gid = 0,
		.perm_owner = FilePermissionTriad::All,
		.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Execute,
		.perm_users = FilePermissionTriad::Read | FilePermissionTriad::Execute,
		.extra = ExtraFileFlags::Directory
	};
}

bool FileSystem::is_file(NodeIdx fid) const
{
	return fid == get_root() || files_.contains(fid);
}

bool FileSystem::is_file(const FilePath& path) const
{
	return get_fid(path) != 0;
}

bool FileSystem::is_dir(NodeIdx fid) const
{
	if (fid == get_root())
		return true;

	if (auto it = metadata_.find(fid); it != metadata_.end())
		return has_flag<ExtraFileFlags, uint8_t>(it->second.extra, ExtraFileFlags::Directory);

	return false;
}

bool FileSystem::is_dir(const FilePath& path) const
{
	return is_dir(get_fid(path));
}

bool FileSystem::is_directory_root(NodeIdx fid) const
{
	if (auto it = roots_.find(fid); it != roots_.end())
		return it->second == fid; // If the root is itself, it counts as a root.
	
	return true;
}

bool FileSystem::is_empty(NodeIdx fid) const
{
	if (!is_dir(fid))
		return true;

	std::vector<NodeIdx> files = get_files(fid);
	return files.empty();
}

std::string_view FileSystem::get_filename(NodeIdx fid) const
{
	if (auto it = fid_to_path_.find(fid); it != fid_to_path_.end())
		return it->second.get_name();

	return {};
}

FilePath FileSystem::get_path(NodeIdx fid) const
{
	if (fid == get_root())
		return {"/"};

	if (auto it = fid_to_path_.find(fid); it != fid_to_path_.end())
		return it->second;

	return {};
}

NodeIdx FileSystem::get_fid(const FilePath& path) const
{
	if (path.is_root_or_empty())
		return get_root();

	if (auto it = path_to_fid_.find(path); it != path_to_fid_.end())
		return it->second;

	return 0;
}

std::size_t FileSystem::get_links(NodeIdx fid)
{
	return mappings_.count(fid);
}

std::size_t FileSystem::get_bytes(NodeIdx fid)
{
	if (auto it = files_.find(fid); it != files_.end())
	{
		return it->second->size();
	}

	return 0;
}

std::string FileSystem::get_flags(NodeIdx fid)
{
	std::string out(10, '-');

	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		const FilePermissionTriad& o = it->second.perm_owner;
		const FilePermissionTriad& g = it->second.perm_group;
		const FilePermissionTriad& u = it->second.perm_users;
		const ExtraFileFlags& e = it->second.extra;

		bool dir = has_flag<ExtraFileFlags, uint8_t>(e, ExtraFileFlags::Directory);
		out[0] = dir ? 'd' : '-';

		bool setuid = has_flag<ExtraFileFlags, uint8_t>(e, ExtraFileFlags::SetUid);
		char uxs = setuid ? 's' : 'x';
		out[1] = has_flag(o, FilePermissionTriad::Read) 	? 'r' : '-';
		out[2] = has_flag(o, FilePermissionTriad::Write) 	? 'w' : '-';
		out[3] = has_flag(o, FilePermissionTriad::Execute) 	? uxs : '-';

		bool setgid = has_flag<ExtraFileFlags, uint8_t>(e, ExtraFileFlags::SetGid);
		char gxs = setgid ? 's' : 'x';
		out[4] = has_flag(g, FilePermissionTriad::Read) 	? 'r' : '-';
		out[5] = has_flag(g, FilePermissionTriad::Write) 	? 'w' : '-';
		out[6] = has_flag(g, FilePermissionTriad::Execute) 	? gxs : '-';

		out[7] = has_flag(u, FilePermissionTriad::Read) 	? 'r' : '-';
		out[8] = has_flag(u, FilePermissionTriad::Write) 	? 'w' : '-';
		out[9] = has_flag(u, FilePermissionTriad::Execute) 	? 'x' : '-';
	}

	return out;
}

std::pair<int32_t, int32_t> FileSystem::get_owner(NodeIdx fid)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		return std::make_pair(it->second.owner_uid, it->second.owner_gid);
	}
	return std::make_pair(0, 0);
}

std::string FileSystem::get_mdate(NodeIdx fid)
{
	
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		std::chrono::system_clock::time_point tp{std::chrono::seconds{it->second.modified}};
		return std::format("{:%Y-%m-%d %X}", tp);
	}

	return "-";
}

uint64_t FileSystem::get_last_modified(NodeIdx fid)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
		return it->second.modified;
		
	return 0;
}

FileMeta* FileSystem::get_metadata(NodeIdx fid)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
		return &it->second;
		
	return nullptr;
}

std::vector<NodeIdx> FileSystem::get_files(NodeIdx dir, bool recurse) const
{
	if (!is_dir(dir))
		return {};

	std::vector<NodeIdx> v{};

	auto range = mappings_.equal_range(dir);
	std::transform(range.first, range.second, std::back_inserter(v), [](std::pair<NodeIdx, NodeIdx> element){ return element.second; });

	if (recurse)
	{
		std::vector<NodeIdx> v2{};
		for (NodeIdx child : v)
		{
			v2.append_range(get_files(child, true));
		}
		v.append_range(v2);
	}

	return v;
}

std::vector<NodeIdx> FileSystem::get_files(const FilePath& path, bool recurse) const
{
	return get_files(get_fid(path), recurse);
}

std::vector<FilePath> FileSystem::get_paths(NodeIdx fid, bool recurse) const
{
	std::vector<FilePath> out;
	std::ranges::transform(get_files(fid, recurse), std::back_inserter(out), [this](NodeIdx node){ return get_path(node); });
	return out;
}

std::vector<FilePath> FileSystem::get_paths(const FilePath& path, bool recurse) const
{
	return get_paths(get_fid(path), recurse);
}

NodeIdx FileSystem::get_parent_folder(NodeIdx fid) const
{
	if (auto it = roots_.find(fid); it != roots_.end())
		return it->second;

	return fid;
}

std::vector<NodeIdx> FileSystem::get_root_chain(NodeIdx fid) const
{
	if (!is_file(fid))
		return {};

	std::vector<NodeIdx> chain = { fid };
	NodeIdx current = fid;

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

FileOpResult FileSystem::create_directory(const FilePath& path, const CreateFileParams& params)
{
	FileOpResult res = create_file(path, params);
	if (auto [fid, ptr, err] = res; err.value() == 0)
	{
		file_set_directory_flag(fid, true);
	}
	return res;
}

NodeIdx FileSystem::create_ensure_path(const FilePath& path, const CreateFileParams& params)
{
	FilePath parent = path.get_parent_path();
	if (NodeIdx parent_fid = get_fid(parent))
	{
		return parent_fid;
	}
	else
	{
		auto [new_parent_fid, ptr, err] = create_directory(parent, params);
		return new_parent_fid;
	}
}

bool FileSystem::remove_file(NodeIdx fid, FileRemoverFn&& func)
{
	FilePath path = get_path(fid);
	return remove_file(path, std::forward<FileRemoverFn>(func));
}

bool FileSystem::remove_file(const FilePath& path, FileRemoverFn&& func)
{
	NodeIdx fid = get_fid(path);

	/* If this is the root directory, ensure we can operate on it. */
	if (path == "/" && !func(*this, path, std::error_condition{EPERM, std::generic_category()}))
	{
		return false;
	}

	/* If this file doesn't exist, report it to callback and return. */
	if (!is_file(fid))
	{
		func(*this, path, std::error_condition{ENOENT, std::generic_category()});
		return false;
	}

	/* If we're not empty, and the callback doesn't say that's okay, fail. */
	if (!(is_empty(fid) || func(*this, path, std::error_condition{ENOTEMPTY, std::generic_category()})))
	{
		return false;
	}

	/* If we get here, the callback must have given green light for recursion.
	Remove all children. */
	for (NodeIdx child : get_files(fid))
	{
		if (!remove_file(child, std::forward<FileRemoverFn>(func)))
		{
			return false;
		}
	}

	/* Even if callback requests resume, fail if not empty after recursion. */
	if (!is_empty(fid))
	{
		func(*this, path, std::error_condition{ENOTEMPTY, std::generic_category()});
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

	return func(*this, path, {});

}

std::error_condition FileSystem::remove_file(NodeIdx fid, bool recurse)
{
	if (!is_file(fid))
		return {ENOENT, std::generic_category()};

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

		return {};
	}
	else if (recurse)
	{
		for (NodeIdx child : get_files(fid))
		{
			if (auto err = remove_file(child, true); err.value() > 0)
				return err;
		}
		return remove_file(fid, false);
	}

	return {ENOTEMPTY, std::generic_category()};
}

std::error_condition FileSystem::remove_file(const FilePath& path, bool recurse)
{
	NodeIdx fid = get_fid(path);
	return remove_file(fid, recurse);
}

FileOpResult FileSystem::get_file(NodeIdx fid, FileAccessFlags flags)
{
	if (is_dir(fid) && has_flag<FileAccessFlags>(flags, FileAccessFlags::Write))
		return std::make_tuple(fid, nullptr, std::error_condition{EINVAL, std::generic_category()});
	
	if (auto it = files_.find(fid); it != files_.end())
		return std::make_tuple(fid, it->second, std::error_condition{});

	return std::make_tuple(0, nullptr, std::error_condition{ENOENT, std::generic_category()});
}

FileOpResult FileSystem::get_file(const FilePath& path, FileAccessFlags flags)
{
	if (NodeIdx fid = get_fid(path); is_file(fid))
		return get_file(fid, flags);

	return std::make_tuple(0, nullptr, std::error_condition{ENOENT, std::generic_category()});
}

File* FileSystem::find(NodeIdx fid)
{
	if (auto it = files_.find(fid); it != files_.end())
		return it->second.get();

	return nullptr;
}

void FileSystem::set_flag(FilePermissionTriad& base, FilePermissionTriad set_flags)
{
	base |= set_flags;
}

void FileSystem::clear_flag(FilePermissionTriad& base, FilePermissionTriad clear_flags)
{
	base = base & ~clear_flags;
}

bool FileSystem::has_flag(const FilePermissionTriad& base, FilePermissionTriad test_flags)
{
	return (base & test_flags) == test_flags;
}

bool FileSystem::file_set_flag(NodeIdx fid, FilePermissionCategory cat, FilePermissionTriad set_flags)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		file_set_flag(it->second, cat, set_flags);
		return true;
	}
	return false;
}

bool FileSystem::file_clear_flag(NodeIdx fid, FilePermissionCategory cat, FilePermissionTriad clear_flags)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		file_clear_flag(it->second, cat, clear_flags);
		return true;
	}
	return false;
}

bool FileSystem::file_has_flag(NodeIdx fid, FilePermissionCategory cat, FilePermissionTriad test_flags) const
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

bool FileSystem::file_has_flag(const FileMeta& meta, FilePermissionCategory cat, FilePermissionTriad test_flags)
{
	switch (cat)
	{
		case FilePermissionCategory::Owner: return has_flag(meta.perm_owner, test_flags);
		case FilePermissionCategory::Group: return has_flag(meta.perm_group, test_flags);
		case FilePermissionCategory::Users: return has_flag(meta.perm_users, test_flags);
	}
	return false;
}

bool FileSystem::file_set_directory_flag(NodeIdx fid, bool new_is_dir)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		if (new_is_dir) 
		{ 
			set_flag<ExtraFileFlags, uint8_t>(it->second.extra, ExtraFileFlags::Directory); 
		}
		else 
		{ 
			clear_flag<ExtraFileFlags, uint8_t>(it->second.extra, ExtraFileFlags::Directory); 
		}
		return true;
	}
	return false;
}

bool FileSystem::file_set_modified_now(NodeIdx fid)
{
	if (auto it = metadata_.find(fid); it != metadata_.end())
	{
		auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
		it->second.modified = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
		return true;
	}
	return false;
}

bool FileSystem::file_set_permissions(NodeIdx fid, FilePermissionTriad owner, FilePermissionTriad group, FilePermissionTriad users)
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

bool FileSystem::file_set_permissions(NodeIdx fid, int32_t owner, int32_t group, int32_t users)
{
	return file_set_permissions(fid, 
		static_cast<FilePermissionTriad>(owner),
		static_cast<FilePermissionTriad>(group),
		static_cast<FilePermissionTriad>(users));
}

bool FileSystem::check_permission(const SessionData& session, NodeIdx fid, FileAccessFlags mode)
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

FileOpResult FileSystem::create_file(const FilePath& path, const CreateFileParams& params)
{
	return create_file<File>(path, params);
}

OpenFileTablePair FileSystem::open_file_entry(NodeIdx node, FileAccessFlags flags)
{
	OpenFileHandle h = get_handle();
	auto [it, success] = open_files_.emplace(h, OpenFileTableEntry
	{
		.node = node,
		.instance_count = 0,
		.flags = flags
	});
	
	if (success)
	{
		//std::println("Opening file {}, inode {}: mode {}.", h, node, static_cast<uint32_t>(flags));
		return std::make_pair(h, &it->second);
	}
	else
	{
		return_handle(h);
		return std::make_pair(-1, nullptr);
	}

}

void FileSystem::close_file_entry(OpenFileHandle h)
{
	if (auto it = open_files_.find(h); it != open_files_.end())
	{
		assert(it->second.instance_count == 0);
		//std::println("Closing file {}.", h);
		open_files_.erase(it);
		return_handle(h);
	}
}

std::expected<size_t, std::error_condition> FileSystem::write(OpenFileHandle h, std::string data)
{
	if (auto it = open_files_.find(h); it != open_files_.end())
	{
		OpenFileTableEntry& entry = it->second;
		File* file = find(entry.node);
		assert(file);

		if (not has_flag<FileAccessFlags>(entry.flags, FileAccessFlags::Write))
			return std::unexpected(std::error_condition{EPERM, std::generic_category()});

		if (has_flag<FileAccessFlags>(entry.flags, FileAccessFlags::Append))
		{
			file->append(std::move(data));
		}
		else
		{
			file->write(std::move(data));
		}

		file_set_modified_now(entry.node);

		return data.size();
	}

	return std::unexpected(std::error_condition{EFAULT, std::generic_category()});
}

std::expected<std::string_view, std::error_condition> FileSystem::read(OpenFileHandle h, size_t bytes)
{
	if (auto it = open_files_.find(h); it != open_files_.end())
	{
		OpenFileTableEntry& entry = it->second;
		File* file = find(entry.node);
		assert(file);

		if (not has_flag<FileAccessFlags>(entry.flags, FileAccessFlags::Read))
			return std::unexpected(std::error_condition{EPERM, std::generic_category()});

		// TODO: Actually do something with the 'bytes' parameter.
		return file->get_view();
	}

	return std::unexpected(std::error_condition{EFAULT, std::generic_category()});
}

std::expected<File*, std::error_condition> FileSystem::get(OpenFileHandle h)
{
	if (auto it = open_files_.find(h); it != open_files_.end())
	{
		OpenFileTableEntry& entry = it->second;
		File* file = find(entry.node);
		assert(file);

		if (not has_flag<FileAccessFlags>(entry.flags, FileAccessFlags::Execute))
			return std::unexpected(std::error_condition{EPERM, std::generic_category()});

		return file;
	}

	return std::unexpected(std::error_condition{EFAULT, std::generic_category()});
}

bool FileSystem::serialize(world::FileSystem* to) const
{
	for (auto&& [h, file] : files_)
	{
		const FileMeta& meta = metadata_.at(h);
		const FilePath& path = fid_to_path_.at(h);

		world::File* arr = to->add_files();
		arr->set_content(file->get_string());
		arr->set_path(path.get_string());

		arr->set_modified(meta.modified); 								//uint64_t modified{0};
		arr->set_owner_uid(meta.owner_uid); 							//int32_t owner_uid{0};
		arr->set_owner_gid(meta.owner_gid); 							//int32_t owner_gid{0};
		arr->set_perm_owner(static_cast<uint32_t>(meta.perm_owner)); 	//FilePermissionTriad perm_owner{7};
		arr->set_perm_group(static_cast<uint32_t>(meta.perm_group)); 	//FilePermissionTriad perm_group{0};
		arr->set_perm_users(static_cast<uint32_t>(meta.perm_users)); 	//FilePermissionTriad perm_users{0};
		arr->set_extra(static_cast<uint32_t>(meta.extra)); 				//ExtraFileFlags extra{};
	}

	return true;
}

bool FileSystem::deserialize(const world::FileSystem& from)
{
	for (auto&& file : from.files())
	{
		FileMeta ar_meta
		{
			.modified = file.modified(),
			.owner_uid = file.owner_uid(),
			.owner_gid = file.owner_gid(),
			.perm_owner = static_cast<FilePermissionTriad>(file.perm_owner()),
			.perm_group = static_cast<FilePermissionTriad>(file.perm_group()),
			.perm_users = static_cast<FilePermissionTriad>(file.perm_users()),
			.extra = static_cast<ExtraFileFlags>(file.extra())
		};

		FilePath path{file.path()};
		if (auto it = path_to_fid_.find(path); it != path_to_fid_.end())
		{
			metadata_[it->second] = ar_meta;
			File* f = find(it->second);
			assert(f);
			f->write(file.content());
		}
		else
		{
			CreateFileParams params
			{
				.recurse = true,
				.meta = ar_meta,
				.content = file.content(),
			};

			create_file(path, std::move(params));
		}
	}

	return true;
}

OpenFileHandle FileSystem::get_handle()
{
	if (free_handles_.empty())
		return handle_counter_++;

	return free_handles_.extract(free_handles_.begin()).value();
}

void FileSystem::return_handle(OpenFileHandle h)
{
	free_handles_.insert(h);
}
