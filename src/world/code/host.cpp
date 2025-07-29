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

Task<bool> Host::start_host()
{
    if (state_ != DeviceState::PoweredOff)
        co_return false;

    std::println("{0} is starting...", hostname_);

    state_ = DeviceState::Starting;

    for (auto& device : devices_)
        co_await device->start_device(this);

    std::optional<File> boot_file = std::invoke([this] 
    {
        return std::nullopt;
        // for (auto& device : devices_)
        // {
        //     if (auto opt = device->read_file("boot.os"); opt.has_value())
        //         return opt;
        // }
    });

    co_await boot_from(boot_file.value_or({}));

    state_ = DeviceState::PoweredOn;

    co_return true;
}

Task<bool> Host::shutdown_host()
{
    std::println("Shutting down {0}...", hostname_);

    state_ = DeviceState::Stopping;

    for (auto& device : devices_)
        co_await device->shutdown_device(this);

    state_ = DeviceState::PoweredOn;

    co_return true;
}

Task<bool> Host::boot_from(const File& boot_file)
{
    /*
    Example boot file:
    path += "bin/exec"
    path += "bin/config"
    path += "bin/calculator"
    path += "usr/webhost"
    path += "usr/proxy"
    path += "usr/firewall"
    path += &.paths;
    config dbc:5:: clock 1.5 ;set cpu clock speed
    exec webhost 8080;
    exec proxy 90;
    exec firewall 500;
    */

    // auto view = boot_file.get_stream();
    // std::string line{};
    // while (std::getline(view, line))
    // {
    //     os_->exec(line);
    // }

    co_return true;
}
