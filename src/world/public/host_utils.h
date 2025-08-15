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

        skel.create_os<T_OS>();

		return world.add_host(std::move(ptr));
	}
}