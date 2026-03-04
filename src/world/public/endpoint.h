#pragma once

#include "addr.h"

#include <string>
#include <string_view>

class IEndpoint
{
public:

	virtual void set_ip(const std::string& new_ip) = 0;
	virtual void set_ip(const Address6& new_ip) = 0;
	virtual const Address6& get_ip() const = 0;

};