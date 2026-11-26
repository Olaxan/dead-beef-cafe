#pragma once

#include "datasource_base.h"

#include <mio/mmap.hpp>

#include <filesystem>

namespace fs = std::filesystem;

class DatasourceFlatbuf : public IDatasourceBase
{
public:

	DatasourceFlatbuf();

	void init_from(fs::path path);

	void open() override;
	void close() override;

	~DatasourceFlatbuf() = default;

protected:

	mio::mmap_source view_;

};