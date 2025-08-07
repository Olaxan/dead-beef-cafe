#include "host.h"

#include "task.h"
#include "proc.h"
#include "world.h"
#include "device.h"
#include "disk.h"
#include "file.h"
#include "filesystem.h"
#include "cpu.h"
#include "nic.h"
#include "os.h"

#include <optional>
#include <sstream>
#include <string>
#include <iostream>
#include <chrono>

Host::Host(World& world, std::string Hostname)
: world_(world), hostname_(Hostname) { }

bool Host::start_host()
{
    if (state_ != DeviceState::PoweredOff)
        return false;

    state_ = DeviceState::Starting;

    for (auto& device : devices_)
        device->start_device(this);

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
