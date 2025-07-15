#pragma once

#include <string>
#include <string_view>
#include <vector>

class OS;
class Host;

struct Command
{
	std::vector<std::string> lex(std::string_view cmd);
	virtual void parse(OS& os, std::string_view cmd) = 0;
};