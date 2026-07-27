#include "host.h"

#include "game_srv.h"
#include "task.h"
#include "proc.h"
#include "device.h"
#include "disk.h"
#include "file.h"
#include "filesystem.h"
#include "cpu.h"
#include "nic.h"
#include "os.h"

#include "proto/host.pb.h"

#include <optional>
#include <sstream>
#include <string>
#include <iostream>
#include <chrono>

Host::Host(GameServices& services, HostContext& ctx, std::string Hostname)
: services_(services), context_(ctx), hostname_(Hostname) 
{ }

Host::~Host() = default;

OS& Host::get_os()
{
	return *os_;
}

bool Host::start_host()
{
    if (state_ != DeviceState::PoweredOff)
        return false;

    state_ = DeviceState::Starting;

    for (auto& device : devices_)
        device->start_device(this);

    os_->start_os();

    state_ = DeviceState::PoweredOn;

    return true;
}

bool Host::shutdown_host()
{
    if (state_ == DeviceState::PoweredOff)
        return false;

    state_ = DeviceState::Stopping;

    for (auto& device : devices_)
        device->shutdown_device(this);

    state_ = DeviceState::PoweredOn;

    return true;
}

void Host::set_os(std::unique_ptr<OS>&& os)
{
	os_ = std::move(os);
}

bool Host::serialize(world::Host* to)
{
	return os_->serialize(to);
}

bool Host::deserialize(const world::Host& from)
{
	return os_->deserialize(from);
}
