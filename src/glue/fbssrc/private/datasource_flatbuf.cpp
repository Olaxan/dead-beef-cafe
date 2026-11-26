#include "datasource_flatbuf.h"

#include <mio/mmap.hpp>

#include <asset_pack_generated.h>

#include <system_error>
#include <iostream>
#include <print>

#include <iso646.h>

void DatasourceFlatbuf::init_from(fs::path path)
{
	std::error_code error;
	if (view_.map(path.string(), error); not error)
	{
		std::println(std::cerr, "Failed to map {}!", path.string());
		return;
	}

	const AssetData* data = GetAssetData(view_.data());
}

void DatasourceFlatbuf::open()
{

}

void DatasourceFlatbuf::close()
{
	
}
