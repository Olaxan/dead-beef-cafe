#pragma once

#include "proc_defs.h"

#include <string>
#include <sstream>
#include <memory>
#include <functional>

using FileWriteCallbackFn = std::function<void(std::string_view)>;

class File : public std::enable_shared_from_this<File>
{
public:

	File() = default;
	
	File(uint64_t fid) : fid_(fid) {};

	virtual ~File() = default;

	void write(ProcessFn exec);
	//void write(ProcessFn&& exec);
	
	/* Write to the file, modifying it. */
	virtual void write(std::string content);
	
	/* Read all content from the file, but do not modify it. */
	virtual std::optional<std::string> read() const;
	
	/* Consume all content of the file. */
	virtual std::optional<std::string> eat();
	
	void append(std::string content);
	
	std::size_t size() const;
	
	std::string_view get_view() const;
	std::stringstream get_stream() const;
	const ProcessFn& get_executable() const;
	
	void add_read_callback(FileWriteCallbackFn&& callback)
    {
		callbacks_.push_back(std::move(callback));
    }
	
	void notify_write();

protected:


	uint64_t fid_{};
	std::string content_{};
	ProcessFn executable_{nullptr};
	std::vector<FileWriteCallbackFn> callbacks_{};

};