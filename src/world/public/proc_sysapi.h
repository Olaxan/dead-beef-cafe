#pragma once

#include "task.h"

#include <vector>
#include <string>
#include <cstdint>

class Proc;

class ProcSysApi
{
public:

	ProcSysApi() = delete;
	ProcSysApi(Proc* owner);
	~ProcSysApi();

	EagerTask<int32_t> exec(std::vector<std::string>&& args);

protected:

	Proc& owner_;
	
};
