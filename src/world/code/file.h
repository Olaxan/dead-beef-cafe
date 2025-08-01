#pragma once

#include <string>
#include <sstream>

enum class FileModeFlags : uint8_t
{
	None = 0,
	Read = 1,
	Write = 2,
	Execute = 4,
	Directory = 8
};

class File
{
public:

	File() = default;
	
	File(uint64_t fid, FileModeFlags flags) 
		: fid_(fid), flags_(flags) {};

	void set_flag(FileModeFlags flags);
	void clear_flag(FileModeFlags flags);
	bool has_flag(FileModeFlags flags);

	std::string_view get_view() const;
	std::stringstream get_stream() const;

private:

	uint64_t fid_{};
	std::string content_{};
	FileModeFlags flags_{FileModeFlags::None};

};