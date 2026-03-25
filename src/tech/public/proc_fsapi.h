#pragma once

#include "proc_types.h"
#include "filesystem_types.h"
#include "filepath.h"
#include "task.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <system_error>
#include <expected>

class Proc;
class OS;
class FileSystem;

using FileQueryResult = std::expected<NodeIdx, std::error_condition>;

class ProcFsApi
{
public:

	ProcFsApi() = delete;
	ProcFsApi(Proc* owner);
	~ProcFsApi();

	void copy_descriptors_from(const ProcFsApi& other);
	void register_descriptors();

	std::expected<FileDescriptor, std::error_condition> open(FilePath path, FileAccessFlags flags);
	std::error_condition close(FileDescriptor fd);
	std::expected<size_t, std::error_condition> write(FileDescriptor fd, std::string data) const;
	std::expected<std::string_view, std::error_condition> read(FileDescriptor fd) const;
	std::expected<ProcessFn, std::error_condition> read_exe(FileDescriptor fd) const;
	std::expected<FileMeta*, std::error_condition> get_metadata(FileDescriptor fd) const;

	FilePath resolve(FilePath path);
	std::expected<NodeIdx, std::error_condition> query(const FilePath& path, FileAccessFlags flags);
	std::error_condition remove(const FilePath& path, bool recurse = false);
	bool remove_using(const FilePath& path, FileRemoverFn&& func);

	OpenFileHandle get_file_handle(FileDescriptor fd) const;
	NodeIdx get_node(FileDescriptor fd) const;
	
	bool check_permission(NodeIdx node, FileAccessFlags mode);

	void close_all();

protected:


protected:

	Proc& proc;
	OS& os;
	FileSystem& fs;

protected:

	std::unordered_map<FileDescriptor, OpenFileTablePair> fd_table_{};


	friend Proc;
};
