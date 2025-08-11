#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <concepts>
#include <print>

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
	bool is_valid_path() const { return valid_; }
	bool is_root_or_empty() const { return path_.empty() || path_ == "/"; }

	std::string_view get_parent_view() const;
	FilePath get_parent_path() const;
	std::string_view get_name() const;
	std::string_view get_view() const { return path_.empty() ? "/" : path_; }
	std::string get_string() const { return path_.empty() ? "/" : path_; }

	void prepend(const FilePath& path);
	void append(const FilePath& path);

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
        return std::formatter<std::string>::format(addr.path_, ctx);
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

enum class FileAccessFlags : uint32_t
{
	None = 0,
	Read = 1,
	Write = 2,
	Create = 4
};

using FileOpResult = std::pair<uint64_t, FileSystemError>;
using FilePtrResult = std::pair<File*, FileSystemError>;
using FileRemoverFn = std::function<bool(const FileSystem&, const FilePath&, FileSystemError)>;

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

	std::vector<uint64_t> get_files(uint64_t fid) const;
	uint64_t get_parent_folder(uint64_t fid) const;
	uint64_t get_root() const { return root_; }
	std::vector<uint64_t> get_root_chain(uint64_t fid) const;

	/* Create a file at the specified location (and fills in missing directories). */
	FileOpResult create_file(const FilePath& path, bool recurse = true);

	/* Create a folder at the specified location (and fills in missing directories). */
	FileOpResult create_directory(const FilePath& path, bool recurse = true);

	/* Walks a provided path and creates all the directories that are missing. 
	Returns the fid of the final folder. */
	uint64_t create_ensure_path(const FilePath& path);

	FileSystemError remove_file(uint64_t fid, bool recurse = false);
	FileSystemError remove_file(const FilePath& path, bool recurse = false);

	/* Version of remove_file that takes a deciding callback, which aborts the operation if returning false.
	If true is returned, the operation should proceed even if an error is reported. */
	bool remove_file(uint64_t fid, FileRemoverFn&& func);
	bool remove_file(const FilePath& path, FileRemoverFn&& func);

	/* Returns a pointer to a file, if found; otherwise nullptr. */
	FilePtrResult open(const FilePath& path, FileAccessFlags flags);

protected:

	template<std::derived_from<File> T>
	FileOpResult add_file(const FilePath& path, FileModeFlags flags = FileModeFlags::None)
	{
		FilePath parent = path.get_parent_path();
		uint64_t parent_fid = get_fid(parent);

		if (parent_fid == 0)
			return std::make_pair(0, FileSystemError::FileNotFound);

		uint64_t fid = ++fid_counter_;
		auto [it, success] = files_.emplace(fid, std::make_unique<T>(fid, flags));

		if (success)
		{
			fid_to_path_[fid] = path;
			path_to_fid_[path] = fid;
			roots_[fid] = parent_fid;
			mappings_.insert(std::make_pair(parent_fid, fid));
		}

		return std::make_pair(fid, FileSystemError::Success);
	}

private:

	const uint64_t root_{1};
	uint64_t fid_counter_{1024};
	std::unordered_map<uint64_t, std::unique_ptr<File>> files_ = {};
	std::unordered_map<uint64_t, FilePath> fid_to_path_{};
	std::unordered_map<FilePath, uint64_t> path_to_fid_{};
	std::unordered_map<uint64_t, uint64_t> roots_{};
	std::unordered_multimap<uint64_t, uint64_t> mappings_{};

	friend class Navigator;
};