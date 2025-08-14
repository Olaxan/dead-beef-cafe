#include "os_fsacc.h"

FileOpResult FileSystemAccessor::open(const FilePath& path, FileAccessFlags flags)
{

	if (uint64_t fid = fs_.get_fid(path))
	{
		
	}

	FileOpResult res = fs_.open(path);

	/* Not equal here! */
	if (flags == FileAccessFlags::Create)
	{
		auto [new_fid, ptr, err] = fs_.create_file(path);
		if (err == FileSystemError::Success)
			return open(path, flags);
	}
}