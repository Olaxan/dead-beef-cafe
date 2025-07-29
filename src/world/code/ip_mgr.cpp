#include "ip_mgr.h"

#include "netw.h"

void IpManager::step(float delta_seconds)
{
	for (auto& stream : streams_)
		std::invoke(stream);
}