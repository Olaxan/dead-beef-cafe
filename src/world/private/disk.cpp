#include "disk.h"

#include "filesystem.h"

Disk::~Disk() = default;

FileSystem& Disk::create_fs()
{
	fs_ = std::make_unique<FileSystem>();
	return *fs_;
}