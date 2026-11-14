#pragma once

#include "os.h"
#include "proc.h"

class BasicOS : public OS
{
public:

	BasicOS();
	~BasicOS();

	virtual void start_os() override;
	virtual void reinstall_os() override;
	
};