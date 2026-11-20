#pragma once

#include "business_types.h"

#include <cstdint>
#include <string>
#include <vector>


struct Business
{
	std::string name{"Business"};
	std::size_t num_employees{1};
	BusinessType type{BusinessType::Computing};

	std::vector<BusinessRolePair> roles;
};