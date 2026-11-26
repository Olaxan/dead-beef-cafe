#pragma once

#include "uid64.h"

class Internet;
class FileArchive;

struct HostContext
{
	Uid64 uid{0};
	Internet* internet{nullptr};
	FileArchive* archive{nullptr};
};