#pragma once

#include "host.h"
#include "world.h"
#include "device.h"
#include "disk.h"
#include "file.h"
#include "filesystem.h"
#include "cpu.h"
#include "nic.h"
#include "os.h"

#include <concepts>

namespace HostUtils
{
	template<std::derived_from<OS> T_OS>
	Host* create_host(World& world, std::string hostname)
	{
		std::unique_ptr<Host> ptr = std::make_unique<Host>(world, hostname);
        Host& skel = *ptr;

        Disk& my_disk = skel.create_device<Disk>(500);
        CPU& my_cpu = skel.create_device<CPU>(1.5f);
        NIC& my_nic = skel.create_device<NIC>(100.f);
        FileSystem& fs = my_disk.create_fs();

        fs.create_directory("/dev");
        fs.create_directory("/bin");
        fs.create_directory("/etc");
        fs.create_directory("/home");
        fs.create_directory("/lib");
        fs.create_directory("/sbin");
        fs.create_directory("/tmp");
        fs.create_directory("/var/log");
        fs.create_directory("/var/lock");
        fs.create_directory("/var/tmp");
        fs.create_directory("/usr/bin");
        fs.create_directory("/usr/man");
        fs.create_directory("/usr/lib");
        fs.create_directory("/usr/local");
        fs.create_directory("/usr/share");

        skel.create_os<T_OS>();

		return world.add_host(std::move(ptr));
	}
}