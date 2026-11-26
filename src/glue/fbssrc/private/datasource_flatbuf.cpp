#include "datasource_flatbuf.h"

#include <mio/mmap.hpp>

#include <asset_pack_generated.h>

#include <system_error>
#include <iostream>
#include <print>

#include <iso646.h>

DatasourceFlatbuf::DatasourceFlatbuf() = default;

void DatasourceFlatbuf::init_from(fs::path path)
{
	std::error_code error;
	if (view_.map(path.string(), error); error)
	{
		std::println(std::cerr, "Failed to map {}!", path.string());
		return;
	}

	std::println("Mapped asset data from '{}' ({} bytes).", path.string(), view_.size());

	data_ = GetAssetData(view_.data());
}

void DatasourceFlatbuf::open()
{

}

void DatasourceFlatbuf::close()
{

}

std::string DatasourceFlatbuf::get_name()
{
	return data_->name()->str();
}
