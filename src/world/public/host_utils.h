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
		std::unique_ptr<OS> os = std::make_unique<T>(std::forward<Args>(args)...);
		os->set_hostname(host.get_hostname());
		host.set_os(std::move(os));
	}

	template<std::derived_from<OS> T_OS>
	void prepare_host(World& world, Host& host)
	{
		host.create_device<Disk>(500);
		host.create_device<CPU>(1.5f);
		host.create_device<NIC>(100.f);
		create_os<T_OS>(world, host);
	}

	template<std::derived_from<OS> T_OS>
	Host* create_host(World& world, std::string hostname)
	{
		std::unique_ptr<Host> ptr = std::make_unique<Host>(hostname);
		Uid64 new_uid = Uid64Generator::generate();

        HostUtils::prepare_host<T_OS>(world, *ptr);

		return world.add_host(new_uid, std::move(ptr));
	}

	void install_os(OS& os);
}