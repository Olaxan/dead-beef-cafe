#pragma once

#include <string>

class IEndpoint
{
public:

	virtual void set_ip(const std::string& new_ip) = 0;
	virtual const std::string& get_ip() const = 0;

};