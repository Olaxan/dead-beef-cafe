#pragma once

#include <any>

class ITimerBase;
class ILinkManager;
class IAudioBase;

struct GameServices
{
	std::any outer;
	ITimerBase& timers;
	IAudioBase& audio;
};