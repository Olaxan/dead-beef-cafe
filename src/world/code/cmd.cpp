#include "cmd.h"

#include <sstream>
#include <iomanip>

std::vector<std::string> Command::lex(std::string_view cmd)
{
	std::string str(cmd); //yuck

	std::string temp{};
	std::vector<std::string> substr;
	
	std::stringstream ss(str);

	while (ss >> std::quoted(temp))
	{
		substr.push_back(std::move(temp));
	}

	return substr;
}