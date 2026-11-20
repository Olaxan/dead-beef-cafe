#pragma once

#include <tuple>
#include <vector>
#include <string>

struct PersonGenerationTemplate
{
	std::string tag{"person_unemployed"};
	std::pair<std::size_t, std::size_t> num_instances_per_country_range;
};