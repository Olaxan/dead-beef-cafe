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
    std::println("{0} is starting...", hostname_);

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

    state_ = true;

    co_return true;
}

Task<bool> Host::shutdown_host()
{
    std::println("Shutting down {0}...", hostname_);

    for (auto& device : devices_)
        co_await device->shutdown_device(this);

    state_ = false;

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

    auto view = boot_file.get_stream();
    std::string line{};
    while (std::getline(view, line))
    {
        os_->exec(line);
    }

    co_return true;
}

// void Host::launch()
// {
//     while (state_)
//     {
//         os_->run();
//         std::this_thread::sleep_for(std::chrono::milliseconds(1));
//     }
// }

std::unique_ptr<Host> Host::create_skeleton_host(World& world, std::string hostname)
{
    auto ptr = std::make_unique<Host>(world, hostname);
	Host& skel = *ptr;
    
	Disk& my_disk = skel.create_device<Disk>(500);
    CPU& my_cpu = skel.create_device<CPU>(1.5f);
    NIC& my_nic = skel.create_device<NIC>(100.f);
    FileSystem& fs = my_disk.create_fs();
    
    fs.add_file("welcome.txt");
    
    std::stringstream boot{};
    boot << "config 0 size auto" << std::endl;
    boot << "mount 0";
    boot << "config 1 clock auto" << std::endl;
    boot << "config 2 band auto" << std::endl;
    boot << "exec welcome.txt";

    fs.add_file("boot.os", boot.str(), FileModeFlags::Execute);

    skel.create_os<OS>();

    return std::move(ptr);
}
