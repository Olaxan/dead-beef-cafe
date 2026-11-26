#pragma once

#include "datasource_base.h"

#include <memory>

class IArchiveBase
{
public:

	virtual void add_datasource(std::shared_ptr<IDatasourceBase> source) = 0;

	virtual ~IArchiveBase() = default;
};