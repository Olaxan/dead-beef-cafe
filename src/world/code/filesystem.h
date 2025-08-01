#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <concepts>

#include "file.h"

class FileSystem
{
public:

	FileSystem();

	/* Returns whether file handle is valid. This is not the opposite of is_dir, as a directory is a file. */
	bool is_file(uint64_t fid) const { return files_.contains(fid); }

	/* Returns whether the specified file is valid, and whether or not it's a directory. */
	bool is_dir(uint64_t fid) const;

	bool is_directory_root(uint64_t fid) const;

	/* Returns the filename of a valid file handle, otherwise an empty string. */
	std::string get_filename(uint64_t fid) const;

	/* Returns the file ID corresponding to a filename. Returns root if no such file exists. */
	uint64_t get_fid(std::string filename) const;

	std::vector<uint64_t> get_files(uint64_t fid) const;
	uint64_t get_parent_folder(uint64_t fid) const;
	uint64_t get_root() const { return root_; }
	std::vector<uint64_t> get_root_chain(uint64_t fid) const;
	char get_drive_letter() const { return 'A'; }

	uint64_t create_file(std::string filename, uint64_t root);
	uint64_t create_directory(std::string filename, uint64_t root);

protected:

	template<std::derived_from<File> T>
	uint64_t add_file(std::string name, uint64_t root = 0, FileModeFlags flags = FileModeFlags::None)
	{
		uint64_t fid = ++fid_counter_;
		auto [it, success] = files_.emplace(fid, std::make_unique<T>(fid, flags));

		if (success)
		{
			fid_to_name_[fid] = name;
			name_to_fid_[name] = fid;
			roots_[fid] = root;
			mappings_.insert(std::make_pair(root, fid));
		}

		return fid;
	}

	template<std::derived_from<File> T>
	uint64_t add_root_file(std::string name, FileModeFlags flags = FileModeFlags::Directory)
	{
		uint64_t fid = ++fid_counter_;
		auto [it, success] = files_.emplace(fid, std::make_unique<T>(fid, flags));

		if (success)
		{
			fid_to_name_[fid] = name;
			name_to_fid_[name] = fid;
		}

		return fid;
	}

private:

	uint64_t root_{0};
	uint64_t fid_counter_{0};
	std::unordered_map<uint64_t, std::unique_ptr<File>> files_ = {};
	std::unordered_map<uint64_t, std::string> fid_to_name_{};
	std::unordered_map<std::string, uint64_t> name_to_fid_{};
	std::unordered_map<uint64_t, uint64_t> roots_{};
	std::unordered_multimap<uint64_t, uint64_t> mappings_{};

	friend class Navigator;
};