#include "disk.h"

#include "filesystem.h"

FileSystem& Disk::create_fs()
{
	fs_ = std::make_unique<FileSystem>();
	return *fs_;
}