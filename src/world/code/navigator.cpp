#include "navigator.h"

#include <sstream>
#include <algorithm>
#include <ranges>

void Navigator::go_up()
{
	current_ = fs_.get_parent_folder(current_);
}

std::vector<std::string> Navigator::get_files() const
{
	if (!fs_.is_dir(current_))
		return { fs_.get_filename(current_) };

	std::vector<std::string> out{};
	for (int64_t child : fs_.get_files(current_))
	{
		out.emplace_back(fs_.get_filename(child));
	}

	return out;
}

std::string Navigator::get_path() const
{
	std::vector<uint64_t> chain = fs_.get_root_chain(current_);
	std::stringstream ss;
	ss << fs_.get_drive_letter() << ":";

	for (uint64_t file : chain)
		ss << "/" << fs_.get_filename(file);

	return ss.str();
}

uint64_t Navigator::create_file(std::string name) const
{
	return fs_.create_file(name, current_);
}

uint64_t Navigator::create_directory(std::string name) const
{
	return fs_.create_directory(name, current_);
}

bool Navigator::go_to(uint64_t dir)
{
	if (!fs_.is_dir(dir))
		return false;

	current_ = dir;
	return false;
}

bool Navigator::enter(std::string dir)
{
	std::vector<uint64_t> near = fs_.get_files(current_);
	uint64_t proxy = fs_.get_fid(dir);
	if (auto it = std::find(near.begin(), near.end(), proxy); it != near.end())
	{
		go_to(*it);
		return true;
	}

	return false;
}
