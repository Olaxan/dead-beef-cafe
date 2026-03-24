#include "proc_fsapi.h"

#include "users_mgr.h"
#include "filesystem.h"
#include "file.h"
#include "proc.h"
#include "os.h"

/* --- Helpers --- */
std::string replace(std::string_view input, std::string_view from, std::string_view to)
{
	return input | std::views::split(from) | std::views::join_with(to) | std::ranges::to<std::string>();
}
/* --- Helpers --- */


ProcFsApi::ProcFsApi(Proc* owner) 
: proc(*owner), os(*proc.owning_os), fs(*os.get_filesystem()) { }

ProcFsApi::~ProcFsApi() = default;

void ProcFsApi::copy_descriptors_from(const ProcFsApi& other)
{
	fd_table_ = other.fd_table_;
}

void ProcFsApi::register_descriptors()
{
	for (auto&& [fd, page] : fd_table_)
	{
		OpenFileTableEntry* entry = page.second;
		++entry->instance_count;
	}
}

std::expected<FileDescriptor, std::error_condition> ProcFsApi::open(FilePath path, FileAccessFlags flags)
{

	if (path.is_relative())
		path.make_absolute();

	if (NodeIdx fid = fs.get_fid(path))
	{
		/* The file exists -- check if we can read it. */
		if (!check_permission(fid, flags))
			return std::unexpected(std::error_condition{EACCES, std::generic_category()});

		auto ret = fs.open_file_entry(fid, flags);
		FileDescriptor fd = proc.get_descriptor();
		++ret.second->instance_count;
		fd_table_[fd] = ret;
		return fd;
	}
	else if (FileSystem::has_flag<FileAccessFlags>(flags, FileAccessFlags::Create))
	{
		/* The file doesn't exist but we want to create it, if possible. */
		FileSystem::CreateFileParams params = {
			.recurse = false,
			.meta = {
				.owner_uid = proc.get_uid(),
				.owner_gid = proc.get_gid(),
				.perm_owner = FilePermissionTriad::Read | FilePermissionTriad::Write,
				.perm_group = FilePermissionTriad::Read | FilePermissionTriad::Write,
				.perm_users = FilePermissionTriad::Read
			}
		}; //TODO: Add a 'mode' parameter to open(), which allows the user to specify this stuff.

		auto [node, ptr, err] = fs.create_file(path, params);
		auto ret = fs.open_file_entry(node, flags);
		FileDescriptor fd = proc.get_descriptor();
		++ret.second->instance_count;
		fd_table_[fd] = ret;
		return fd;
	}

	/* The file was not found -- return nothing. */
	return std::unexpected(std::error_condition{ENOENT, std::generic_category()});
}

std::error_condition ProcFsApi::close(FileDescriptor fd)
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		OpenFileHandle h = pair.first;
		OpenFileTableEntry* entry = pair.second;

		if (--entry->instance_count == 0)
		{
			fs.close_file_entry(h);
		}

		fd_table_.erase(it);
		proc.return_descriptor(fd);

		return {};
	}

	return std::error_condition{EBADF, std::generic_category()};
}

void ProcFsApi::close_all()
{
	while (!fd_table_.empty())
	{
		auto first = fd_table_.begin();
		if (auto exp_close = close(first->first); exp_close.value() != 0)
		{
			proc.errln("proc: Failed to close file: {}.", exp_close.message());
		}
	}
}

std::expected<size_t, std::error_condition> ProcFsApi::write(FileDescriptor fd, std::string data) const
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		OpenFileHandle h = pair.first;

		return fs.write(h, data);
	}
	
	return std::unexpected{std::error_condition{EBADF, std::generic_category()}};
}

std::expected<std::string_view, std::error_condition> ProcFsApi::read(FileDescriptor fd) const
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		OpenFileHandle h = pair.first;
		
		return fs.read(h, 0); // TODO: Add support for reading some bytes only.
	}
	
	return std::unexpected{std::error_condition{EBADF, std::generic_category()}};
}

std::expected<ProcessFn, std::error_condition> ProcFsApi::read_exe(FileDescriptor fd) const
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		OpenFileHandle h = pair.first;
		
		if (auto exp_file = fs.get(h))
		{
			File* file = exp_file.value();
			return file->get_executable();
		}
	}
	
	return std::unexpected{std::error_condition{EBADF, std::generic_category()}};
}

std::expected<FileMeta*, std::error_condition> ProcFsApi::get_metadata(FileDescriptor fd) const
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		OpenFileTableEntry* entry = pair.second;
		
		FileMeta* meta = fs.get_metadata(entry->node);

		if (meta) { return meta; }

		return std::unexpected{std::error_condition{EIO, std::generic_category()}};
	}
	
	return std::unexpected{std::error_condition{EBADF, std::generic_category()}};
}

OpenFileHandle ProcFsApi::get_file_handle(FileDescriptor fd) const
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		return pair.first;
	}

	return -1;
}

NodeIdx ProcFsApi::get_node(FileDescriptor fd) const
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		return pair.second->node;
	}

	return -1;
}

bool ProcFsApi::check_permission(NodeIdx node, FileAccessFlags mode)
{
	UsersManager& users = *os.get_users_manager();

	int32_t uid = proc.get_uid();
	int32_t gid = proc.get_gid();

	if (uid == 0 || gid == 0)
		return true;

	if (FileMeta* meta_ptr = fs.get_metadata(node))
	{
		FileMeta& meta = *meta_ptr;

		bool group_match = (gid == meta.owner_gid || users.check_belongs(uid, meta.owner_gid));
		bool owner_match = (uid == meta.owner_uid);
		bool users_match = true;

		bool read_valid = std::invoke([&]() -> bool
		{
			if (!FileSystem::has_flag<FileAccessFlags>(mode, FileAccessFlags::Read))
				return true;

			if (FileSystem::file_has_flag(meta, FilePermissionCategory::Users, FilePermissionTriad::Read)) return true;
			if (FileSystem::file_has_flag(meta, FilePermissionCategory::Owner, FilePermissionTriad::Read) && owner_match) return true;
			if (FileSystem::file_has_flag(meta, FilePermissionCategory::Group, FilePermissionTriad::Read) && group_match) return true;

			return false;
		});

		bool write_valid = std::invoke([&]() -> bool
		{
			if (!FileSystem::has_flag<FileAccessFlags>(mode, FileAccessFlags::Write))
				return true;

			if (FileSystem::file_has_flag(meta, FilePermissionCategory::Users, FilePermissionTriad::Write)) return true;
			if (FileSystem::file_has_flag(meta, FilePermissionCategory::Owner, FilePermissionTriad::Write) && owner_match) return true;
			if (FileSystem::file_has_flag(meta, FilePermissionCategory::Group, FilePermissionTriad::Write) && group_match) return true;

			return false;
		});

		bool exec_valid = std::invoke([&]() -> bool
		{
			if (!FileSystem::has_flag<FileAccessFlags>(mode, FileAccessFlags::Execute))
				return true;

			if (FileSystem::file_has_flag(meta, FilePermissionCategory::Users, FilePermissionTriad::Execute)) return true;
			if (FileSystem::file_has_flag(meta, FilePermissionCategory::Owner, FilePermissionTriad::Execute) && owner_match) return true;
			if (FileSystem::file_has_flag(meta, FilePermissionCategory::Group, FilePermissionTriad::Execute) && group_match) return true;

			return false;
		});

		return read_valid && write_valid && exec_valid;
	}
	
	return false;
}

FilePath ProcFsApi::resolve(FilePath path)
{
	FilePath pwd(proc.get_var("PWD"));
	std::string_view home = proc.get_var_or("HOME", "/");

	/* cd ~/../hello */
	path.substitute("~", home);
	/* cd /usr/home/fredr/../hello */
	
	/* cd ../../hello */
	if (path.is_relative())
		path.make_absolute(pwd);
	/* cd /usr/home/fredr/../../hello */

	/* Split the path into segments, removing delimitors, filtering out empty segments. */
	auto&& split_range = path.get_view() | std::views::split('/') | std::views::filter([](const auto& f){ return !std::string_view(f).empty(); });
	/* usr, home, fredr, .., .., hello */

	std::deque<std::string_view> path_stack{};

	for (auto&& part : split_range)
	{
		std::string_view dir(part);

		if (dir == "..")
		{
			path_stack.pop_back();
			continue;
		}
		
		if (dir == ".")
			continue;

		path_stack.push_back(dir);
	}
	
	FilePath out{ path_stack | std::views::join_with('/') | std::ranges::to<std::string>() };
	out.make_absolute();
	return out;
}

FileQueryResult ProcFsApi::query(const FilePath& path, FileAccessFlags flags)
{
	if (NodeIdx fid = fs.get_fid(path))
	{
		/* The file exists -- check if we can read it. */
		if (!check_permission(fid, flags))
			return std::unexpected{std::error_condition{EACCES, std::generic_category()}};

		return fid;
	}
	else if (FileSystem::has_flag<FileAccessFlags>(flags, FileAccessFlags::Create))
	{
		return std::unexpected{std::error_condition{EINVAL, std::generic_category()}};
	}

	/* The file was not found -- return nothing. */
	return std::unexpected{std::error_condition{ENOENT, std::generic_category()}};
}

std::error_condition ProcFsApi::remove(const FilePath& path, bool recurse)
{
	if (NodeIdx fid = fs.get_fid(path))
	{
		if (!check_permission(fid, FileAccessFlags::Read | FileAccessFlags::Execute))
			return std::error_condition{EACCES, std::generic_category()};

		return fs.remove_file(path, recurse);
	}

	return std::error_condition{ENOENT, std::generic_category()};
}

bool ProcFsApi::remove_using(const FilePath& path, FileRemoverFn&& func)
{
	if (NodeIdx fid = fs.get_fid(path))
	{
		if (!check_permission(fid, FileAccessFlags::Write | FileAccessFlags::Execute))
		{
			func(fs, path, std::error_condition{EACCES, std::generic_category()});
			return false;
		}

		return fs.remove_file(path, std::move(func));
	}

	func(fs, path, std::error_condition{ENOENT, std::generic_category()});
	return false;
}