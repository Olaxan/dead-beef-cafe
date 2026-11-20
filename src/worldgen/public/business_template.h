#pragma once

#include "business_types.h"

#include <tuple>
#include <vector>
#include <string>

using NumericRange = std::pair<std::size_t, std::size_t>;

struct BusinessGenerationTemplate
{
	std::string tag{"business_garage"};
	NumericRange num_employees_range{1, 2};
	NumericRange num_instances_per_country_range{5, 10};
	std::vector<BusinessRole> roles;
};