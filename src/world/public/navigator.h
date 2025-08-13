#pragma once

#include "filesystem.h"

#include <vector>
#include <string>

class Navigator
{
public:

	Navigator(FileSystem& sys) : fs_(sys) 
	{ 
		current_ = fs_.get_root();
	}

	std::vector<std::string_view> get_files() const;
	FilePath get_path() const;
	std::string_view get_dir() const;
	uint64_t get_current() const { return current_; }

	FileOpResult create_file(FilePath path) const;
	FileOpResult create_directory(FilePath path) const;

	FileSystemError remove_file(FilePath name, bool recurse = false) const;
		
	bool go_up();
	bool go_to(FilePath path);
	bool go_to(uint64_t fid);

private:

	FileSystem& fs_;
	uint64_t current_{0};
};