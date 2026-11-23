#pragma once

#include "tag_provider.h"

class Country : public ITagProvider<std::string>
{
public:

	std::string name{"Unnamed"};


protected:
	
};