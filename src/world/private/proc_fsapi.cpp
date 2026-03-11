#include "proc_fsapi.h"

ProcFsApi::ProcFsApi(Proc* owner) 
: owner_(owner) { }

ProcFsApi::~ProcFsApi() = default;