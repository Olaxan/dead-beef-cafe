#pragma once

#include "filesystem.h"

class Proc;

namespace FileUtils
{
	FilePath resolve(const Proc& proc, const FilePath& path);
	FileQueryResult query(const Proc& proc, const FilePath& path, FileAccessFlags flags);
	FileOpResult open(const Proc& proc, const FilePath& path, FileAccessFlags flags);
	FileSystemError remove(const Proc& proc, const FilePath& path, bool recurse = false);
	bool remove(const Proc& proc, const FilePath& path, FileRemoverFn&& func);
};