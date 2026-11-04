#pragma once

#include "uid64.h"

class Internet;

struct HostContext
{
	Uid64 uid{0};
	Internet& internet;
};