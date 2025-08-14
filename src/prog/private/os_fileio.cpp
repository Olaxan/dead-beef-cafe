#include "os_fileio.h"

#include "proc.h"
#include "os.h"

FileOpResult FileUtils::open(const Proc& proc, const FilePath& path, FileAccessFlags flags)
{

	OS& os = *proc.owning_os;
	FileSystem& fs = *os.get_filesystem();
	const SessionData& session = proc.get_session();

	if (uint64_t fid = fs.get_fid(path))
	{
		if (!fs.check_permission(session, fid, flags))
			return std::make_tuple(0, nullptr, FileSystemError::InsufficientPermissions);

		return fs.open(fid);
	}
	else if (FileSystem::has_flag<FileAccessFlags>(flags, FileAccessFlags::Create))
	{
		// if (!fs.check_permission(session, fid, FileAccessFlags::Create))
		// 	return std::make_tuple(0, nullptr, FileSystemError::InsufficientPermissions);

		return fs.create_file(path);
	}

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
		if (!fs.check_permission(session, fid, FileAccessFlags::Read | FileAccessFlags::Execute))
		{
			func(fs, path, FileSystemError::InsufficientPermissions);
			return false;
		}

		return fs.remove_file(path, std::move(func));
	}

	func(fs, path, FileSystemError::FileNotFound);
	return false;
}
