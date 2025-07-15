#pragma once

#include "cmd.h"

class OS;
class Host;

struct CmdEcho : public Command
{
	virtual void parse(OS& runner, std::string_view cmd) override;
};

struct CmdShutdown : public Command
{
	virtual void parse(OS& runner, std::string_view cmd) override;
};