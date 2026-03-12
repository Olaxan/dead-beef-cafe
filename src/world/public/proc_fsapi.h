#pragma once

#include "proc_types.h"
#include "filesystem_types.h"
#include "filepath.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <system_error>
#include <expected>

class Proc;
class OS;
class FileSystem;

class ProcFsApi
{
public:

	ProcFsApi() = delete;
	ProcFsApi(Proc* owner);
	~ProcFsApi();

	std::expected<FileDescriptor, std::error_condition> open(FilePath path, FileAccessFlags flags);
	std::error_condition close(FileDescriptor fd);
	std::expected<size_t, std::error_condition> write(FileDescriptor fd, std::string data);
	std::expected<std::string_view, std::error_condition> read(FileDescriptor fd);
	std::expected<ProcessFn, std::error_condition> read_exe(FileDescriptor fd);
	std::expected<FileMeta*, std::error_condition> get_metadata(FileDescriptor fd);

	bool check_permission(NodeIdx node, FileAccessFlags mode);

	void close_all();

protected:

	Proc* owner_{nullptr};
	OS* os_{nullptr};
	FileSystem* fs_{nullptr};

	std::unordered_map<FileDescriptor, OpenFileTablePair> fd_table_{};

	friend Proc;
};
