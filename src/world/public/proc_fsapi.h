#pragma once

#include <cstdint>
#include <memory>

class Proc;

class ProcFsApi
{
public:

	ProcFsApi() = delete;
	ProcFsApi(Proc* owner);
	~ProcFsApi();

protected:

	Proc* owner_{nullptr};


	friend Proc;
};
