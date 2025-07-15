#pragma once

#include <vector>
#include <memory>

#include "file.h"

class FileSystem
{
public:

	FileSystem() = default;

	template<typename T = File>
	T& add_file(std::string path, FileModeFlags flags = FileModeFlags::None)
	{
		auto ptr = std::make_unique<T>(path, std::string(), flags);
		T& out = *ptr;
		files_.push_back(std::move(ptr));
		return out;
	}

	template<typename T = File>
	T& add_file(std::string path, std::string content, FileModeFlags flags = FileModeFlags::None)
	{
		auto ptr = std::make_unique<T>(path, content, flags);
		T& out = *ptr;
		files_.push_back(std::move(ptr));
		return out;
	}

private:

	std::vector<std::unique_ptr<File>> files_ = {};
};