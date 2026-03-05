#pragma once

#include "proc.h"

#include <string>
#include <sstream>
#include <memory>

using FileWriteCallbackFn = std::function<void(std::string_view)>;

class File : public std::enable_shared_from_this<File>
{
public:

	File() = default;
	
	File(uint64_t fid) : fid_(fid) {};

	void write(ProcessFn exec);
	//void write(ProcessFn&& exec);
	void write(std::string content);
	void append(std::string content);

	/* Consume all content of the file. */
	std::string eat();

	std::size_t size() const;
	
	std::string_view get_view() const;
	std::stringstream get_stream() const;
	const ProcessFn& get_executable() const;

	void add_callback(FileWriteCallbackFn&& callback)
    {
        callbacks_.push_back(std::move(callback));
    }

protected:

	void notify_write();

private:

	uint64_t fid_{};
	std::string content_{};
	ProcessFn executable_{nullptr};
	std::vector<FileWriteCallbackFn> callbacks_{nullptr};

};