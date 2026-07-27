#pragma once

#include "os.h"
#include "host.h"
#include "world.h"
#include "device.h"
#include "disk.h"
#include "file.h"
#include "filesystem.h"
#include "cpu.h"
#include "nic.h"
#include "uid64.h"

#include <concepts>

namespace HostUtils
{
	template<std::derived_from<OS> T, typename ...Args>
	void create_os(World& world, Host& host, Args&& ...args)
	{
		std::unique_ptr<OS> os = std::make_unique<T>(world.get_services(), world.get_host_context(), std::forward<Args>(args)...);
		os->set_hostname(host.get_hostname());
		host.set_os(std::move(os));
	}

	template<std::derived_from<OS> T_OS>
	Host* create_host(World& world, std::string hostname)
	{
		std::unique_ptr<Host> ptr = std::make_unique<Host>(world.get_services(), world.get_host_context(), hostname);
        Host& skel = *ptr;

        Disk& my_disk = skel.create_device<Disk>(500);
        CPU& my_cpu = skel.create_device<CPU>(1.5f);
        NIC& my_nic = skel.create_device<NIC>(100.f);

		std::size_t name_hash = std::hash<std::string>{}(hostname);
		Uid64 id{static_cast<uint64_t>(name_hash)};

        HostUtils::create_os<T_OS>(world, skel);

		return world.add_host(id, std::move(ptr));
	}

	void install_os(OS& os);
}