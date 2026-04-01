#pragma once

#include "task.h"
#include "filepath.h"
#include "proc_types.h"

#include <vector>
#include <string>
#include <cstdint>

class Proc;
class OS;
class FileSystem;

struct ExecParams
{
	ReaderFn reader{nullptr};
	WriterFn writer{nullptr};
	InvokeFn invoke{nullptr};
	bool run_in_background{false};
	bool is_tty{true};
};

class ProcSysApi
{
public:

	ProcSysApi() = delete;
	ProcSysApi(Proc* owner);
	~ProcSysApi();

	Task<int32_t> exec(FilePath path, std::vector<std::string>&& args, ExecParams&& params = {});
	Task<int32_t> exec(std::vector<std::string>&& args);
	Task<int32_t> exec(std::string argstr);

protected:

	Proc& proc;
	OS& os;
	FileSystem& fs;
	
};
