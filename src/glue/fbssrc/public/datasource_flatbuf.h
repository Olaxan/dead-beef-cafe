#pragma once

#include "datasource_base.h"

#include <mio/mmap.hpp>

#include <filesystem>

namespace fs = std::filesystem;

struct AssetData;

class DatasourceFlatbuf : public IDatasourceBase
{
public:

	DatasourceFlatbuf();

	void init_from(fs::path path);

	void open() override;
	void close() override;

	std::string get_name() override;

	~DatasourceFlatbuf() = default;

protected:

	mio::mmap_source view_;
	const AssetData* data_;

};