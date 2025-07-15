#pragma once

#include <string>
#include <sstream>

enum class FileModeFlags : uint8_t
{
	None = 0,
	Read = 1,
	Write = 2,
	Execute = 4
};

class File
{
public:

	File() = default;
	
	File(std::string path, FileModeFlags flags) 
		: path_(path), flags_(flags) {};

	File(std::string path, std::string content, FileModeFlags flags) 
		: path_(path), content_(content), flags_(flags) {};

	std::string_view get_view() const;
	std::stringstream get_stream() const;

private:

	std::string path_{};
	std::string content_{};
	FileModeFlags flags_{FileModeFlags::None};

};