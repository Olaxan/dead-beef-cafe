#pragma once

#include <string>

class IDatasourceBase
{
public:

	virtual void open() = 0;
	virtual void close() = 0;

	virtual std::string get_name() = 0;

	virtual ~IDatasourceBase() = default;
};