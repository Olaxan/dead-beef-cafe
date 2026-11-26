#pragma once

#include <algorithm>
#include <iterator>
#include <random>
#include <string>
#include <iostream>

template<typename URBG = std::mt19937>
class RandChars
{
public:

	RandChars() = default;

	std::string get(std::size_t length)
	{
		std::string result;
		result.resize(length);
		std::sample(std::cbegin(charset_), 
					std::cend(charset_),
					std::begin(result),
					std::intptr_t(length),
					std::forward<URBG>(rand_));

		return result;
	}

	std::string operator ()(std::size_t length)
	{
		return get(length);
	}

protected:

	URBG rand_;

	static constexpr char charset_[] = 
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	
};