#pragma once

#include "person_types.h"

struct BusinessId
{
	int32_t id{-1};
	auto operator <=> (const BusinessId&) const = default;
};

template <>
struct std::hash<BusinessId>
{
  	std::size_t operator()(const BusinessId& k) const
	{
    	return std::hash<int32_t>()(k.id);
	}
};

enum class BusinessType : uint8_t
{
	None = 0,
	Manufacturing,
	Computing,
	CyberSec,
	FinTech,
	ResourceExtraction,
	WeaponsResearch,
	Pharmaceutical
};

enum class BusinessSize : uint8_t
{
	Garage = 0,
	Small,
	Medium,
	Large,
	Enterprise
};

enum class BusinessRole : uint8_t
{
	None = 0,
	Owner,
	CEO,
	CSO,
	CTO,
	CFO,
	Admin
};

using BusinessRolePair = std::pair<PersonId, BusinessRole>;