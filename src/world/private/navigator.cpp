#include "navigator.h"

#include <sstream>
#include <algorithm>
#include <ranges>
#include <print>

bool Navigator::go_up()
{
	int64_t old = current_;
	current_ = fs_.get_parent_folder(current_);
	return old != current_;
}

std::vector<std::string_view> Navigator::get_files() const
{
	if (!fs_.is_dir(current_))
		return { fs_.get_filename(current_) };

	std::vector<std::string_view> out{};
	for (int64_t child : fs_.get_files(current_))
	{
		out.emplace_back(fs_.get_filename(child));
	}

	return out;
}

FilePath Navigator::get_path() const
{
	return fs_.get_path(current_);
}

std::string_view Navigator::get_dir() const
{
	return fs_.get_filename(current_);
}

FileOpResult Navigator::create_file(FilePath path) const
{
	if (path.is_relative())
		path.prepend(get_path());
	
	return fs_.create_file(path);
}

FileOpResult Navigator::create_directory(FilePath path) const
{
	if (path.is_relative())
		path.prepend(get_path());
	
	FileOpResult ret = fs_.create_file(path);
	if (auto [fid, ptr, err] = ret; err == FileSystemError::Success)
	{
		fs_.file_set_flag(fid, FileModeFlags::Directory);
	}
	return ret;
}

FileSystemError Navigator::remove_file(FilePath path, bool recurse) const
{
	if (path.is_relative())
		path.prepend(get_path());

	return fs_.remove_file(path, recurse);
}

bool Navigator::go_to(uint64_t dir)
{
	if (!fs_.is_dir(dir))
		return false;
	
	current_ = dir;

	return true;
}

bool Navigator::go_to(FilePath path)
{
	if (path.is_relative())
		path.prepend(get_path());

	if (uint64_t fid = fs_.get_fid(path))
		return go_to(fid);

	return false;
}
