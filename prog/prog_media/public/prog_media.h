#pragma once

#include "os.h"
#include "proc.h"

#include <coroutine>
#include <vector>

#include <iso646.h>

namespace Programs
{
	ProcessTask CmdRadio(Proc& proc, std::vector<std::string> args);
	ProcessTask CmdSpeak(Proc& proc, std::vector<std::string> args);
};