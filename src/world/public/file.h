#pragma once

#include "proc.h"

#include <string>
#include <sstream>
#include <memory>

class File : public std::enable_shared_from_this<File>
{
public:

	File() = default;
	
	File(uint64_t fid) : fid_(fid) {};

	void write(ProcessFn exec);
	//void write(ProcessFn&& exec);
	void write(std::string content);
	void append(std::string content);

	std::size_t size() const;
	
	std::string_view get_view() const;
	std::stringstream get_stream() const;
	const ProcessFn& get_executable() const;

private:

	uint64_t fid_{};
	std::string content_{};
	ProcessFn executable_{nullptr};

};