#pragma once

#include "context_provider.h"

class Locale : public IContextProvider<std::string>
{
public:

	std::string name{"Unnamed"};


protected:
	
};