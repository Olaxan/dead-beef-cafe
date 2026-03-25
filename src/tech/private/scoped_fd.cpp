#include "scoped_fd.h"

#include "proc.h"
#include "proc_fsapi.h"

#include <print>

FileScope::FileScope(Proc& proc, FilePath path, FileAccessFlags flags)
: proc_(proc)
{
	if (auto exp_fd = proc.fs.open(path, flags))
	{
		fd_ = exp_fd.value();
	}
	else
	{
		proc.errln("Failed to open '{}': {}.", path, exp_fd.error().message());
	}
}

FileScope::FileScope(FileScope&& other)
: proc_(other.proc_), fd_(other.fd_)
{
	other.reset();
}

FileScope::~FileScope()
{
	if (fd_ != -1)
	{
		proc_.fs.close(fd_);
	}
}

std::expected<size_t, std::error_condition> FileScope::write(std::string data) const
{
	return proc_.fs.write(fd_, data);
}

std::expected<std::string_view, std::error_condition> FileScope::read() const
{
	return proc_.fs.read(fd_);
}

void FileScope::reset()
{
	fd_ = -1;
}
