#include "proc_fsapi.h"

#include "users_mgr.h"
#include "filesystem.h"
#include "file.h"
#include "proc.h"
#include "os.h"

ProcFsApi::ProcFsApi(Proc* owner) 
: owner_(owner), os_(owner->owning_os), fs_(os_->get_filesystem()) { }

ProcFsApi::~ProcFsApi() = default;

std::expected<FileDescriptor, std::error_condition> ProcFsApi::open(FilePath path, FileAccessFlags flags)
{
	OS& os = *os_;
	Proc& proc = *owner_;
	FileSystem& fs = *fs_;

	if (path.is_relative())
		path.make_absolute();

	if (NodeIdx fid = fs.get_fid(path))
	{
		/* The file exists -- check if we can read it. */
		if (!check_permission(fid, flags))
			return std::unexpected(std::error_condition{EACCES, std::generic_category()});

		auto ret = fs.open_file_entry(fid, flags);
		FileDescriptor fd = proc.get_descriptor();
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

		fs_->close_file_entry(h);
		fd_table_.erase(it);
		owner_->return_descriptor(fd);

		return {};
	}

	return std::error_condition{EBADF, std::generic_category()};
}

std::expected<size_t, std::error_condition> ProcFsApi::write(FileDescriptor fd, std::string data)
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		OpenFileHandle h = pair.first;

		return fs_->write(h, data);
	}
	
	return std::unexpected{std::error_condition{EBADF, std::generic_category()}};
}

std::expected<std::string_view, std::error_condition> ProcFsApi::read(FileDescriptor fd)
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		OpenFileHandle h = pair.first;
		
		return fs_->read(h, 0); // TODO: Add support for reading some bytes only.
	}
	
	return std::unexpected{std::error_condition{EBADF, std::generic_category()}};
}

std::expected<ProcessFn, std::error_condition> ProcFsApi::read_exe(FileDescriptor fd)
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		OpenFileHandle h = pair.first;
		
		if (auto exp_file = fs_->get(h))
		{
			File* file = exp_file.value();
			return file->get_executable();
		}
	}
	
	return std::unexpected{std::error_condition{EBADF, std::generic_category()}};
}

std::expected<FileMeta*, std::error_condition> ProcFsApi::get_metadata(FileDescriptor fd)
{
	if (auto it = fd_table_.find(fd); it != fd_table_.end())
	{
		const OpenFileTablePair& pair = it->second;
		OpenFileTableEntry* entry = pair.second;
		
		FileMeta* meta = fs_->get_metadata(entry->node);

		if (meta) { return meta; }

		return std::unexpected{std::error_condition{EIO, std::generic_category()}};
	}
	
	return std::unexpected{std::error_condition{EBADF, std::generic_category()}};
}

bool ProcFsApi::check_permission(NodeIdx node, FileAccessFlags mode)
{
	Proc& proc = *owner_;
	OS& os = *proc.owning_os;
	FileSystem& fs = *os.get_filesystem();
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

Task<int32_t> ProcFsApi::close_all()
{
	int32_t errc{0};
	while (!fd_table_.empty())
	{
		auto first = fd_table_.begin();
		if (auto exp_close = close(first->first); exp_close.value() != 0)
		{
			owner_->errln("proc: Failed to close file: {}.", exp_close.message());
			errc++;
		}
	}
	co_return errc;
}
