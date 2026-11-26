#pragma once

class IDatasourceBase
{
public:

	virtual void open() = 0;
	virtual void close() = 0;

	virtual ~IDatasourceBase() = default;
};