#pragma once

#include "filesystem_types.h"
#include "filepath.h"

#include <expected>
#include <system_error>

class Proc;
class FileScope
{
public:

	FileScope(Proc& proc, FilePath path, FileAccessFlags flags);
	FileScope(FileScope&) = delete;
	FileScope(FileScope&& other);
	~FileScope();

	std::expected<size_t, std::error_condition> write(std::string data) const;
	std::expected<std::string_view, std::error_condition> read() const;

	void reset();

protected:

	Proc& proc_;
	FileDescriptor fd_{-1};
};