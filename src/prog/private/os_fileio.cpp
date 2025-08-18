#include "os_fileio.h"

#include "proc.h"
#include "os.h"

std::string replace(std::string_view input, std::string_view from, std::string_view to)
{
	return input | std::views::split(from) | std::views::join_with(to) | std::ranges::to<std::string>();
}

FilePath FileUtils::resolve(const Proc& proc, FilePath path)
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

FileQueryResult FileUtils::query(const Proc& proc, const FilePath& path, FileAccessFlags flags)
{
	OS& os = *proc.owning_os;
	FileSystem& fs = *os.get_filesystem();

	if (uint64_t fid = fs.get_fid(path))
	{
		/* The file exists -- check if we can read it. */
		if (!FileUtils::check_permission(proc, fid, flags))
			return std::make_pair(0, FileSystemError::InsufficientPermissions);

		return std::make_pair(fid, FileSystemError::Success);
	}
	else if (FileSystem::has_flag<FileAccessFlags>(flags, FileAccessFlags::Create))
	{
		return std::make_pair(0, FileSystemError::InvalidFlags);
	}

	/* The file was not found -- return nothing. */
	return std::make_pair(0, FileSystemError::FileNotFound);
}

FileOpResult FileUtils::open(const Proc& proc, const FilePath& path, FileAccessFlags flags)
{

	OS& os = *proc.owning_os;
	FileSystem& fs = *os.get_filesystem();

	if (uint64_t fid = fs.get_fid(path))
	{
		/* The file exists -- check if we can read it. */
		if (!FileUtils::check_permission(proc, fid, flags))
			return std::make_tuple(0, nullptr, FileSystemError::InsufficientPermissions);

		return fs.open(fid, flags);
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
		};

		return fs.create_file(path, params);
	}

	/* The file was not found -- return nothing. */
	return std::make_tuple(0, nullptr, FileSystemError::FileNotFound);
}

FileSystemError FileUtils::remove(const Proc& proc, const FilePath& path, bool recurse)
{
	OS& os = *proc.owning_os;
	FileSystem& fs = *os.get_filesystem();

	if (uint64_t fid = fs.get_fid(path))
	{
		if (!FileUtils::check_permission(proc, fid, FileAccessFlags::Read | FileAccessFlags::Execute))
			return FileSystemError::InsufficientPermissions;

		return fs.remove_file(path, recurse);
	}

	return FileSystemError::FileNotFound;
}

bool FileUtils::remove(const Proc& proc, const FilePath& path, FileRemoverFn&& func)
{
	OS& os = *proc.owning_os;
	FileSystem& fs = *os.get_filesystem();

	if (uint64_t fid = fs.get_fid(path))
	{
		if (!FileUtils::check_permission(proc, fid, FileAccessFlags::Write | FileAccessFlags::Execute))
		{
			func(fs, path, FileSystemError::InsufficientPermissions);
			return false;
		}

		return fs.remove_file(path, std::move(func));
	}

	func(fs, path, FileSystemError::FileNotFound);
	return false;
}

bool FileUtils::check_permission(const Proc& proc, uint64_t fid, FileAccessFlags mode)
{
	OS& os = *proc.owning_os;
	FileSystem& fs = *os.get_filesystem();
	SessionManager& sess = *os.get_session_manager();
	UsersManager& users = *os.get_users_manager();

	int32_t uid = proc.get_uid();
	int32_t gid = proc.get_gid();

	if (FileMeta* meta_ptr = fs.get_metadata(fid))
	{
		FileMeta& meta = *meta_ptr;

		bool group_match = (gid == meta.owner_gid || users.check_belongs(uid, gid));
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

