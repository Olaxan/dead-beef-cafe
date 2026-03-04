#pragma once

#include "os.h"
#include "proc.h"

#include <coroutine>
#include <string>
#include <vector>

namespace ShellUtils
{
	EagerTask<int32_t> Exec(Proc& proc, std::vector<std::string>&& args);
};