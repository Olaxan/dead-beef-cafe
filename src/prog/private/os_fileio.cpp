#include "os_fileio.h"

#include "proc.h"
#include "os.h"

std::string replace(std::string_view input, std::string_view from, std::string_view to)
{
	return input | std::views::split(from) | std::views::join_with(to) | std::ranges::to<std::string>();
}

FilePath FileUtils::resolve(const Proc& proc, const FilePath& path)
{
	FilePath pwd(proc.get_var("PWD"));
	std::string_view home = proc.get_var("HOME");
	
	std::string a = replace(path.get_string(), "..", pwd.get_parent_view()); // Not great but for now...
	std::string b = replace(a, ".", pwd.get_view());
	std::string c = replace(b, "~", home);
	FilePath out{c};
	if (out.is_relative()) { out.prepend(pwd); }
	return out;
}

FileQueryResult FileUtils::query(const Proc& proc, const FilePath& path, FileAccessFlags flags)
{
	OS& os = *proc.owning_os;
	FileSystem& fs = *os.get_filesystem();
	const SessionData& session = proc.get_session();

	if (uint64_t fid = fs.get_fid(path))
	{
		/* The file exists -- check if we can read it. */
		if (!fs.check_permission(session, fid, flags))
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
	const SessionData& session = proc.get_session();

	if (uint64_t fid = fs.get_fid(path))
	{
		/* The file exists -- check if we can read it. */
		if (!fs.check_permission(session, fid, flags))
			return std::make_tuple(0, nullptr, FileSystemError::InsufficientPermissions);

		return fs.open(fid, flags);
	}
	else if (FileSystem::has_flag<FileAccessFlags>(flags, FileAccessFlags::Create))
	{
		/* The file doesn't exist but we want to create it, if possible. */
		FileSystem::CreateFileParams params = {
			.recurse = false,
			.meta = {
				.owner_uid = session.uid,
				.owner_gid = session.gid,
				.perm_owner = FilePermissionTriad::All,
				.perm_group = FilePermissionTriad::Read,
				.perm_users = FilePermissionTriad::None
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
	const SessionData& session = proc.get_session();

	if (uint64_t fid = fs.get_fid(path))
	{
		if (!fs.check_permission(session, fid, FileAccessFlags::Read | FileAccessFlags::Execute))
			return FileSystemError::InsufficientPermissions;

		return fs.remove_file(path, recurse);
	}

	return FileSystemError::FileNotFound;
}

bool FileUtils::remove(const Proc& proc, const FilePath& path, FileRemoverFn&& func)
{
	OS& os = *proc.owning_os;
	FileSystem& fs = *os.get_filesystem();
	const SessionData& session = proc.get_session();

	if (uint64_t fid = fs.get_fid(path))
	{
		if (!fs.check_permission(session, fid, FileAccessFlags::Write | FileAccessFlags::Execute))
		{
			func(fs, path, FileSystemError::InsufficientPermissions);
			return false;
		}

		return fs.remove_file(path, std::move(func));
	}

	func(fs, path, FileSystemError::FileNotFound);
	return false;
}
