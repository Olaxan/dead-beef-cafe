#pragma once

#include <string>
#include <string_view>
#include <print>

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