#pragma once

#include "os.h"
#include "proc.h"

class BasicOS : public OS
{
public:

	BasicOS() = delete;
	BasicOS(GameServices& services, HostContext& ctx);
	~BasicOS();

	virtual void start_os() override;
	virtual void reinstall_os() override;
	
};