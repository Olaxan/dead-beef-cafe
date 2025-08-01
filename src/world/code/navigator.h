#pragma once

#include "filesystem.h"

#include <vector>
#include <string>

class Navigator
{
public:

	Navigator(FileSystem& sys)
		: fs_(sys) 
	{ 
		current_ = fs_.get_root();
	}

	std::vector<std::string> get_files() const;
	std::string get_path() const;
	uint64_t create_file(std::string name) const;
	uint64_t create_directory(std::string name) const;
		
	void go_up();
	bool go_to(std::string dir);
	bool go_to(uint64_t fid);

	bool enter(std::string dir);

private:

	FileSystem& fs_;
	uint64_t current_{0};
};