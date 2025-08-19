#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <concepts>
#include <print>
#include <tuple>
#include <chrono>

#include "file.h"

class FileSystem;

class FilePath
{
public:

	FilePath(std::string path);
	FilePath(const char* path) : FilePath(std::string(path)) {}
	FilePath(std::string_view path) : FilePath(std::string(path)) {}
	FilePath() : FilePath("") {}

	auto operator <=> (const FilePath& other) const
	{
		return path_ <=> other.path_;
	}

	bool operator == (const FilePath& other) const 
	{
        return this->path_ == other.path_;
    }

	bool is_relative() const { return relative_; }
	bool is_absolute() const { return !relative_; }
	bool is_valid_path() const { return valid_; }
	bool is_root_or_empty() const { return path_.empty() || path_ == "/"; }
	bool is_backup() const;
	bool is_config() const;

	std::string_view get_parent_view() const;
	FilePath get_parent_path() const;
	std::string_view get_name() const;
	std::string_view get_view() const { return path_.empty() ? "/" : std::string_view(path_); }
	std::string get_string() const { return path_.empty() ? "/" : path_; }

	void prepend(const FilePath& path);
	void append(const FilePath& path);
	void substitute(std::string_view from, std::string_view to);
	void make_absolute(std::string_view from_dir = {});
	void make_absolute(const FilePath& from_dir);

private:

	void make();

private:

	bool valid_{false};
	bool relative_{false};
	std::string path_{};

	friend struct std::hash<FilePath>;
	friend struct std::formatter<FilePath>;
};

template <>
struct std::hash<FilePath>
{
  	std::size_t operator()(const FilePath& k) const
	{
    	return std::hash<std::string>()(k.path_);
	}
};

template<>
struct std::formatter<FilePath> : std::formatter<std::string> 
{
    auto format(const FilePath& addr, auto& ctx) const
	{
        return std::formatter<std::string>::format(addr.get_string(), ctx);
    }
};

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


class FileSystem
{
public:

	FileSystem();

	static const char* get_fserror_name(FileSystemError code);

	/* Returns whether file handle is valid. This is not the opposite of is_dir, as a directory is a file. */
	bool is_file(uint64_t fid) const;
	bool is_file(const FilePath& path) const;

	/* Returns whether the specified file is valid, and whether or not it's a directory. */
	bool is_dir(uint64_t fid) const;
	bool is_dir(const FilePath& path) const;

	/* Returns whether the specified file has a parent, or if it is a tree root. */
	bool is_directory_root(uint64_t fid) const;

	/* Returns whether the specified file has children, or if it is empty/a non-directory file. */
	bool is_empty(uint64_t fid) const;

	/* Returns the filename of a valid file handle, otherwise an empty string. */
	std::string_view get_filename(uint64_t fid) const;

	/* Returns the path of a valid file handle, or an empty path. */
	FilePath get_path(uint64_t fid) const;

	/* Returns the file ID corresponding to a fully-qualified path. Returns 0 if no such file exists.
	Returns root id for empty or single-slash paths. */
	uint64_t get_fid(const FilePath& path) const;

	/* Return the number of children of this node. */
	std::size_t get_links(uint64_t fid);

	/* Returns the number of bytes of this file. */
	std::size_t get_bytes(uint64_t fid);

	/* Returns a string representation of bits set on this file. */
	std::string get_flags(uint64_t fid);

	/* Returns a pair containing the owning UID and GID of this file. */
	std::pair<int32_t, int32_t> get_owner(uint64_t fid);

	/* Returns a string describing the last-modified date of this file. */
	std::string get_mdate(uint64_t fid);

	/* Returns the last-modified date (seconds since epoch). */
	uint64_t get_last_modified(uint64_t fid);

	/* Returns metadata for a file id, or nullptr if the file does not exist. */
	FileMeta* get_metadata(uint64_t fid);

	std::vector<uint64_t> get_files(uint64_t fid, bool recurse = false) const;
	std::vector<uint64_t> get_files(const FilePath& path, bool recurse = false) const;

	std::vector<FilePath> get_paths(uint64_t fid, bool recurse = false) const;
	std::vector<FilePath> get_paths(const FilePath& path, bool recurse = false) const;

	uint64_t get_parent_folder(uint64_t fid) const;
	uint64_t get_root() const { return root_; }
	std::vector<uint64_t> get_root_chain(uint64_t fid) const;

	struct CreateFileParams
	{
		bool recurse{false};
		FileMeta meta{};
		std::string content{};
		ProcessFn executable{nullptr};
	};

	/* Create a file at the specified location (and optionally fills in missing directories). */
	FileOpResult create_file(const FilePath& path, const CreateFileParams& params);

	/* Create a directory at the specified location (and optionally fills in missing directories). */
	FileOpResult create_directory(const FilePath& path, const CreateFileParams& params);

	/* Walks a provided path and creates all the directories that are missing. 
	Returns the fid of the final folder. */
	uint64_t create_ensure_path(const FilePath& path, const CreateFileParams& params);

	FileSystemError remove_file(uint64_t fid, bool recurse = false);
	FileSystemError remove_file(const FilePath& path, bool recurse = false);

	/* Version of remove_file that takes a deciding callback, which aborts the operation if returning false.
	If true is returned, the operation should proceed even if an error is reported. */
	bool remove_file(uint64_t fid, FileRemoverFn&& func);
	bool remove_file(const FilePath& path, FileRemoverFn&& func);

	/* Returns a pointer to a file, if found; otherwise nullptr. */
	FileOpResult open(uint64_t fid, FileAccessFlags flags = FileAccessFlags::All);
	FileOpResult open(const FilePath& path, FileAccessFlags flags = FileAccessFlags::All);

	static void set_flag(FilePermissionTriad& base, FilePermissionTriad set_flags);
	static void clear_flag(FilePermissionTriad& base, FilePermissionTriad clear_flags);
	static bool has_flag(const FilePermissionTriad& base, FilePermissionTriad test_flags);

	template <typename T, typename Base = uint32_t>
	static void set_flag(T& base, T set_flags)
	{
		base = static_cast<T>(static_cast<Base>(base) | static_cast<Base>(set_flags));
	}

	template <typename T, typename Base = uint32_t>
	static void clear_flag(T& base, T clear_flags)
	{
		base = static_cast<T>(static_cast<Base>(base) & ~static_cast<Base>(clear_flags));
	}

	template <typename T, typename Base = uint32_t>
	static bool has_flag(const T& base, T test_flags)
	{
		return static_cast<T>(static_cast<Base>(base) & static_cast<Base>(test_flags)) == test_flags;
	}
	

	bool file_set_flag(uint64_t fid, FilePermissionCategory cat, FilePermissionTriad set_flags);
	bool file_clear_flag(uint64_t fid, FilePermissionCategory cat, FilePermissionTriad clear_flags);
	bool file_has_flag(uint64_t fid, FilePermissionCategory cat, FilePermissionTriad test_flags) const;

	static void file_set_flag(FileMeta& meta, FilePermissionCategory cat, FilePermissionTriad set_flags);
	static void file_clear_flag(FileMeta& meta, FilePermissionCategory cat, FilePermissionTriad clear_flags);
	static bool file_has_flag(const FileMeta& meta, FilePermissionCategory cat, FilePermissionTriad test_flags);

	bool file_set_directory_flag(uint64_t fid, bool new_is_dir);
	bool file_set_modified_now(uint64_t fid);
	bool file_set_permissions(uint64_t fid, FilePermissionTriad owner, FilePermissionTriad group, FilePermissionTriad users);
	bool file_set_permissions(uint64_t fid, int32_t owner, int32_t group, int32_t users);

	bool check_permission(const SessionData& session, uint64_t fid, FileAccessFlags mode);

	template<std::derived_from<File> T>
	FileOpResult add_file(const FilePath& path, const FileMeta& meta)
	{
		FilePath parent = path.get_parent_path();
		uint64_t parent_fid = get_fid(parent);

		if (parent_fid == 0)
			return std::make_tuple(0, nullptr, FileSystemError::FileNotFound);

		uint64_t fid = ++fid_counter_;
		auto [it, success] = files_.emplace(fid, std::make_shared<T>(fid));

		if (success)
		{
			fid_to_path_[fid] = path;
			path_to_fid_[path] = fid;
			roots_[fid] = parent_fid;
			metadata_[fid] = meta;
			mappings_.insert(std::make_pair(parent_fid, fid));

			file_set_modified_now(fid);

			return std::make_tuple(fid, it->second, FileSystemError::Success);
		}

		return std::make_tuple(fid, nullptr, FileSystemError::IOError);
	}

private:

	const uint64_t root_{1};
	uint64_t fid_counter_{1024};
	std::unordered_map<uint64_t, std::shared_ptr<File>> files_ = {};
	std::unordered_map<uint64_t, FilePath> fid_to_path_{};
	std::unordered_map<FilePath, uint64_t> path_to_fid_{};
	std::unordered_map<uint64_t, uint64_t> roots_{};
	std::unordered_map<uint64_t, FileMeta> metadata_{};
	std::unordered_multimap<uint64_t, uint64_t> mappings_{};

	friend class Navigator;
};