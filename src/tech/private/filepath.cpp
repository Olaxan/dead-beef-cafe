#include "filepath.h"

#include <ranges>
#include <algorithm>

/* --- File Path --- */

FilePath::FilePath(std::string path)
{
	path_ = path.empty() ? "/" : path;
	make();
}

void FilePath::make()
{
	valid_ = false;

	if (path_.empty())
		return;

	relative_ = (path_[0] != '/');

	path_ = path_
	| std::views::split('/') 
	| std::views::filter([](auto&& elem) { return !elem.empty(); })
	| std::views::join_with('/')
	| std::ranges::to<std::string>();

	if (!relative_)
		path_.insert(0, "/");

	// if (path_.back() == '/')
	// 	path_.pop_back();

	valid_ = !path_.empty();
}

bool FilePath::is_backup() const
{
	if (path_.empty()) return false;
	std::string_view name = get_name();
	return name.back() == '~';
}

bool FilePath::is_config() const
{
	if (path_.empty()) return false;
	std::string_view name = get_name();
	return name.front() == '.';
}

std::string_view FilePath::get_parent_view() const
{
	std::string_view path(path_);
	if (!path.empty() && path.back() == '/')
        path.remove_suffix(1);

    // Find last slash
    auto last_slash = path.rfind('/');
    if (last_slash == std::string_view::npos)
        return {}; // No parent

    return path.substr(0, last_slash);
}

FilePath FilePath::get_parent_path() const
{
	return FilePath(get_parent_view());
}

std::string_view FilePath::get_name() const
{
	std::string_view path(path_);

	if (!path.empty() && path.back() == '/')
        path.remove_suffix(1);

    // Find last slash
    auto last_slash = path.rfind('/');
    if (last_slash == std::string_view::npos)
        return {}; // No parent

    return path.substr(last_slash + 1);
}

void FilePath::prepend(const FilePath& other)
{
	if (!other.is_valid_path())
		return;

	const std::string& other_path = other.path_;
	path_ = std::format("{}/{}", other_path, path_);
	make();
}

void FilePath::append(const FilePath& other)
{
	if (!other.is_valid_path())
		return;

	const std::string& other_path = other.path_;
	path_ = std::format("{}/{}", path_, other_path);
	make();
}

void FilePath::substitute(std::string_view from, std::string_view to)
{
	if (from.empty())
		return;

	path_ = (path_ | std::views::split(from) | std::views::join_with(to) | std::ranges::to<std::string>());
	make();
}

void FilePath::make_absolute(std::string_view from_dir)
{
	if (from_dir.length() > 0)
	{
		path_ = std::format("{}/{}", from_dir, path_);
	}

	if (is_relative())
		path_.insert(0, "/");

	make();
}

void FilePath::make_absolute(const FilePath& from_dir)
{
	make_absolute(from_dir.get_view());
}