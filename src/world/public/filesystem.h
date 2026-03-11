#pragma once

#include "file.h"
#include "filepath.h"
#include "filesystem_types.h"

#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <concepts>
#include <tuple>
#include <chrono>

struct SessionData;

class FileSystem
{
public:

	FileSystem();

	static const char* get_fserror_name(FileSystemError code);

	/* Returns whether file handle is valid. This is not the opposite of is_dir, as a directory is a file. */
	bool is_file(NodeIdx fid) const;
	bool is_file(const FilePath& path) const;

	/* Returns whether the specified file is valid, and whether or not it's a directory. */
	bool is_dir(NodeIdx fid) const;
	bool is_dir(const FilePath& path) const;

	/* Returns whether the specified file has a parent, or if it is a tree root. */
	bool is_directory_root(NodeIdx fid) const;

	/* Returns whether the specified file has children, or if it is empty/a non-directory file. */
	bool is_empty(NodeIdx fid) const;

	/* Returns the filename of a valid file handle, otherwise an empty string. */
	std::string_view get_filename(NodeIdx fid) const;

	/* Returns the path of a valid file handle, or an empty path. */
	FilePath get_path(NodeIdx fid) const;

	/* Returns the file ID corresponding to a fully-qualified path. Returns 0 if no such file exists.
	Returns root id for empty or single-slash paths. */
	NodeIdx get_fid(const FilePath& path) const;

	/* Return the number of children of this node. */
	std::size_t get_links(NodeIdx fid);

	/* Returns the number of bytes of this file. */
	std::size_t get_bytes(NodeIdx fid);

	/* Returns a string representation of bits set on this file. */
	std::string get_flags(NodeIdx fid);

	/* Returns a pair containing the owning UID and GID of this file. */
	std::pair<int32_t, int32_t> get_owner(NodeIdx fid);

	/* Returns a string describing the last-modified date of this file. */
	std::string get_mdate(NodeIdx fid);

	/* Returns the last-modified date (seconds since epoch). */
	uint64_t get_last_modified(NodeIdx fid);

	/* Returns metadata for a file id, or nullptr if the file does not exist. */
	FileMeta* get_metadata(NodeIdx fid);

	std::vector<NodeIdx> get_files(NodeIdx fid, bool recurse = false) const;
	std::vector<NodeIdx> get_files(const FilePath& path, bool recurse = false) const;

	std::vector<FilePath> get_paths(NodeIdx fid, bool recurse = false) const;
	std::vector<FilePath> get_paths(const FilePath& path, bool recurse = false) const;

	NodeIdx get_parent_folder(NodeIdx fid) const;
	NodeIdx get_root() const { return root_; }
	std::vector<NodeIdx> get_root_chain(NodeIdx fid) const;

	struct CreateFileParams
	{
		bool recurse{false};
		FileMeta meta{};
		std::string content{};
		ProcessFn executable{nullptr};
	};

	/* Create a directory at the specified location (and optionally fills in missing directories). */
	FileOpResult create_directory(const FilePath& path, const CreateFileParams& params);

	/* Walks a provided path and creates all the directories that are missing. 
	Returns the fid of the final folder. */
	NodeIdx create_ensure_path(const FilePath& path, const CreateFileParams& params);

	FileSystemError remove_file(NodeIdx fid, bool recurse = false);
	FileSystemError remove_file(const FilePath& path, bool recurse = false);

	/* Version of remove_file that takes a deciding callback, which aborts the operation if returning false.
	If true is returned, the operation should proceed even if an error is reported. */
	bool remove_file(NodeIdx fid, FileRemoverFn&& func);
	bool remove_file(const FilePath& path, FileRemoverFn&& func);

	/* Returns a pointer to a file, if found; otherwise nullptr. */
	FileOpResult open(NodeIdx fid, FileAccessFlags flags = FileAccessFlags::All);
	FileOpResult open(const FilePath& path, FileAccessFlags flags = FileAccessFlags::All);
	
	/* Basic find function for low-level code, without file scope. Prefer 'open' to using this. */
	File* find(NodeIdx fid);

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
	

	bool file_set_flag(NodeIdx fid, FilePermissionCategory cat, FilePermissionTriad set_flags);
	bool file_clear_flag(NodeIdx fid, FilePermissionCategory cat, FilePermissionTriad clear_flags);
	bool file_has_flag(NodeIdx fid, FilePermissionCategory cat, FilePermissionTriad test_flags) const;

	static void file_set_flag(FileMeta& meta, FilePermissionCategory cat, FilePermissionTriad set_flags);
	static void file_clear_flag(FileMeta& meta, FilePermissionCategory cat, FilePermissionTriad clear_flags);
	static bool file_has_flag(const FileMeta& meta, FilePermissionCategory cat, FilePermissionTriad test_flags);

	bool file_set_directory_flag(NodeIdx fid, bool new_is_dir);
	bool file_set_modified_now(NodeIdx fid);
	bool file_set_permissions(NodeIdx fid, FilePermissionTriad owner, FilePermissionTriad group, FilePermissionTriad users);
	bool file_set_permissions(NodeIdx fid, int32_t owner, int32_t group, int32_t users);

	bool check_permission(const SessionData& session, NodeIdx fid, FileAccessFlags mode);

	template<std::derived_from<File> T>
	FileOpResult add_file(const FilePath& path, const FileMeta& meta)
	{
		FilePath parent = path.get_parent_path();
		NodeIdx parent_fid = get_fid(parent);

		if (parent_fid == 0)
			return std::make_tuple(0, nullptr, FileSystemError::FileNotFound);

		NodeIdx fid = ++fid_counter_;
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

	template<std::derived_from<File> T>
	FileOpResult create_file(const FilePath& path, const CreateFileParams& params)
	{
		if (get_fid(path))
			return std::make_tuple(0, nullptr, FileSystemError::FileExists);

		if (params.recurse)
			create_ensure_path(path, params);

		auto res = add_file<T>(path, params.meta);
		if (auto [fid, ptr, err] = res; err == FileSystemError::Success)
		{
			if (!params.content.empty()) 
				ptr->write(params.content);
				
			if (params.executable)
				ptr->write(params.executable);
		}
		return res;
	}

	FileOpResult create_file(const FilePath& path, const CreateFileParams& params);


private:

	const NodeIdx root_{1};
	NodeIdx fid_counter_{1024};
	std::unordered_map<NodeIdx, std::shared_ptr<File>> files_ = {};
	std::unordered_map<NodeIdx, FilePath> fid_to_path_{};
	std::unordered_map<FilePath, NodeIdx> path_to_fid_{};
	std::unordered_map<NodeIdx, NodeIdx> roots_{};
	std::unordered_map<NodeIdx, FileMeta> metadata_{};
	std::unordered_multimap<NodeIdx, NodeIdx> mappings_{};

	friend class Navigator;
};