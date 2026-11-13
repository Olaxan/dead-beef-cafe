#pragma once

#include "os.h"
#include "proc.h"

#include <coroutine>
#include <vector>

#include <iso646.h>

namespace Programs
{
	ProcessTask CmdSave(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdLoad(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdGen(Proc& proc, std::vector<std::string> args);
};