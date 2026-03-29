#pragma once

#include "os.h"
#include "proc.h"

class BasicOS : public OS
{
public:

	BasicOS() = delete;
	BasicOS(Host& owner);
	~BasicOS();

	virtual void start_os() override;
	
};