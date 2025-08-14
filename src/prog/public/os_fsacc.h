#pragma once

#include "filesystem.h"
#include "proc.h"
#include "os.h"

enum class FileAccessFlags : uint32_t
{
	None = 0,
	Read = 1,
	Write = 2,
	Create = 4
};

inline FileAccessFlags operator | (FileAccessFlags a, FileAccessFlags b)
{
    return static_cast<FileAccessFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline FileAccessFlags operator & (FileAccessFlags a, FileAccessFlags b)
{
    return static_cast<FileAccessFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline FileAccessFlags& operator |= (FileAccessFlags& a, FileAccessFlags b)
{
    return a = a | b;
}


class FileSystemAccessor
{
public:

	FileSystemAccessor(const Proc& proc)
	: proc_(proc), os_(*proc_.owning_os), fs_(*os_.get_filesystem()) { }

	FileOpResult open(const FilePath& path, FileAccessFlags flags);

private:

	const Proc& proc_;
	const OS& os_;
	FileSystem& fs_;

};