#pragma once

#include <array>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

#include <format>

#include <random>
#include <sstream>
#include <iomanip>
#include <array>
#include <string>

class Uid64 
{
public:

	union
	{
		std::array<uint8_t, 8> bytes{};
		uint64_t num;
	};

	Uid64() = default;

	Uid64(uint64_t num) 
		: num(num) { }

    explicit Uid64(const std::string& address) 
	{
        parse(address);
    }

	auto operator<=>(const Uid64& other) const 
	{
        return num <=> other.num;
    }

    bool operator==(const Uid64& other) const 
	{
        return this->num == other.num;
    }

private:

    void parse(const std::string& address) 
	{
        std::vector<std::string> parts;
        size_t double_colon = address.find("::");

        if (double_colon != std::string::npos) 
		{
            std::string before = address.substr(0, double_colon);
            std::string after = address.substr(double_colon + 2);

            split(before, parts);
            size_t head_count = parts.size();
            split(after, parts);

            size_t tail_count = parts.size() - head_count;
            size_t missing = 8 - (head_count + tail_count);

            if (missing < 0)
                throw std::invalid_argument("Too many octets in IPv6 address");

            parts.insert(parts.begin() + head_count, missing, "0");
        } 
		else 
		{
            split(address, parts);
            if (parts.size() != 8)
                throw std::invalid_argument("Invalid IPv6 address format");
        }

        for (size_t i = 0; i < parts.size(); ++i) 
		{
            uint16_t value = std::stoi(parts[i], nullptr, 16);
            bytes[i * 2] = static_cast<uint8_t>(value >> 8);
            bytes[i * 2 + 1] = static_cast<uint8_t>(value & 0xFF);
        }
    }

    void split(const std::string& str, std::vector<std::string>& out) 
	{
        std::stringstream ss(str);
        std::string segment;
        while (std::getline(ss, segment, ':')) 
		{
            if (!segment.empty())
                out.push_back(segment);
        }
    }
};

template<>
struct std::formatter<Uid64> : std::formatter<std::string> 
{
    auto format(const Uid64& addr, auto& ctx) const
	{
        std::ostringstream oss;
        for (size_t i = 0; i < 8; i += 2) 
		{
            uint16_t segment = (addr.bytes[i] << 8) | addr.bytes[i + 1];
            oss << std::hex << std::setw(4) << std::setfill('0') << segment;
            if (i < 6) oss << ":";
        }

        return std::formatter<std::string>::format(oss.str(), ctx);
    }
};


template <>
struct std::hash<Uid64>
{
  	std::size_t operator()(const Uid64& k) const
	{
    	return std::hash<uint64_t>()(k.num);
	}
};

class UUIDGenerator 
{
public:

    static Uid64 generate() 
	{
		Uid64 out;
        std::array<uint8_t, 8>& bytes = out.bytes;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        for (auto& byte : bytes) {
            byte = static_cast<uint8_t>(dis(gen));
        }

		return out;
    }
};