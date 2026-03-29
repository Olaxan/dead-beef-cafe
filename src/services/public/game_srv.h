#pragma once

#include <any>

class ITimerBase;
class ILinkManager;

struct GameServices
{
	std::any outer;
	ITimerBase& timers;
};