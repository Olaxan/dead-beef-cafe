#pragma once

#include <cstdint>
#include <functional>
#include <tuple>
#include <memory>

class File;
class FileSystem;
class FilePath;

enum class FileSystemError : uint32_t
{
	Success = 0,
	FileNotFound,
	FileExists,
	InsufficientPermissions,
	InvalidFlags,
	FolderNotEmpty,
	IOError,
	PreserveRoot,
	Other
};


/* --- File Permission Category --- */

enum class FilePermissionCategory : uint8_t
{
	Owner,
	Group,
	Users
};


/* --- File Permission Triad --- */

enum class FilePermissionTriad : uint8_t
{
	None 			= 0,
	Read		= 1 << 0,
	Write		= 1 << 1,
	Execute		= 1 << 2,
	All 		= Read | Write | Execute
};

enum class ExtraFileFlags : uint8_t
{
	None 		= 0,
	Directory 	= 1 << 0,
	SetUid 		= 1 << 1,
	SetGid 		= 1 << 2,
	Sticky 		= 1 << 3,
};

inline FilePermissionTriad operator | (FilePermissionTriad a, FilePermissionTriad b)
{
    return static_cast<FilePermissionTriad>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline FilePermissionTriad operator & (FilePermissionTriad a, FilePermissionTriad b)
{
    return static_cast<FilePermissionTriad>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline FilePermissionTriad operator ~ (FilePermissionTriad& a)
{
    return static_cast<FilePermissionTriad>(~static_cast<uint8_t>(a));
}

inline FilePermissionTriad& operator |= (FilePermissionTriad& a, FilePermissionTriad b)
{
    return a = a | b;
}


/* --- File Access Flags --- */

enum class FileAccessFlags : uint32_t
{
	None = 0,
	Read = 1,
	Write = 2,
	Execute = 4,
	Create = 8,
	All = Read | Write | Execute | Create
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


/* --- Definitions --- */

using FileOpResult = std::tuple<uint64_t, std::shared_ptr<File>, FileSystemError>;
using FileQueryResult = std::pair<uint64_t, FileSystemError>;
using FileRemoverFn = std::function<bool(const FileSystem&, const FilePath&, FileSystemError)>;
using NodeIdx = int64_t;


/* --- File Meta-data --- */

struct FileMeta
{
	uint64_t modified{0};
	int32_t owner_uid{0};
	int32_t owner_gid{0};
	FilePermissionTriad perm_owner{7};
	FilePermissionTriad perm_group{0};
	FilePermissionTriad perm_users{0};
	ExtraFileFlags extra{};
};