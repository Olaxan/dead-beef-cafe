#pragma once

#include "proc.h"

#include <string>
#include <sstream>

enum class FileModeFlags : uint8_t
{
	None = 0,
	Read = 1,
	Write = 2,
	Execute = 4,
	Directory = 8,
	System = 16
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

	void write(ProcessFn&& exec);
	void write(std::string content);
	void append(std::string content);
	
	std::string_view get_view() const;
	std::stringstream get_stream() const;
	const ProcessFn& get_executable() const;

private:

	uint64_t fid_{};
	std::string content_{};
	ProcessFn executable_{nullptr};
	FileModeFlags flags_{FileModeFlags::None};

};