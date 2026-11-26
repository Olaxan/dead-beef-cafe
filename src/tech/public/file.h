#pragma once

#include "file_archive_types.h"

#include <string>
#include <sstream>
#include <memory>
#include <optional>

class File : public std::enable_shared_from_this<File>
{
public:

	File() = default;
	
	File(uint64_t fid) : fid_(fid) {};

	virtual ~File() = default;

	/* Write to the file, modifying it.
	Returns the size of the file. */
	virtual std::size_t write(std::string content);
	
	/* Read all content from the file, but do not modify it. */
	virtual std::optional<std::string> read() const;
	
	/* Consume all content of the file. */
	virtual std::optional<std::string> eat();
	
	void append(std::string content);
	
	std::size_t size() const;
	
	std::string_view get_view() const;
	std::stringstream get_stream() const;
	const std::string& get_string() const;

	std::optional<ArHandle> get_link() const;
	void set_link(ArHandle h);
	void set_link_and_clear(ArHandle h);

protected:

	ArHandle link_{};
	uint64_t fid_{};
	std::string content_{};

};