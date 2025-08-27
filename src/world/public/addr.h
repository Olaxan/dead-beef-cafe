#pragma once

#include <array>
#include <vector>
#include <string>
#include <format>
#include <sstream>
#include <iomanip>
#include <expected>
#include <algorithm>
#include <stdexcept>
#include <string_view>

class Address6 {
public:

	union
	{
        char raw[16];
		std::array<uint8_t, 16> bytes{};
		struct
		{
			uint64_t head;
			uint64_t tail;
		};
	};

	Address6() = default;

	Address6(uint64_t pre, uint64_t post) noexcept
		: head(pre), tail(post)	{ }

    explicit Address6(std::array<uint8_t, 16>&& raw_bytes) noexcept;

	auto operator<=>(const Address6& other) const 
	{
        if (auto cmp = head <=> other.head; cmp != 0)
            return cmp;
        return tail <=> other.tail;
    }

    bool operator==(const Address6& other) const 
	{
        return this->head == other.head && this->tail == other.tail;
    }

    std::string_view view() const
    {
        return std::string_view{raw};
    }

    static std::expected<Address6, std::invalid_argument> from_string(const std::string& address) noexcept;
    static std::expected<Address6, std::invalid_argument> from_bytes(std::string_view address) noexcept;

private:
    
    static void split(const std::string& str, std::vector<std::string>& out);

};

template<>
struct std::formatter<Address6> : std::formatter<std::string> 
{
    auto format(const Address6& addr, auto& ctx) const
	{
        std::ostringstream oss;
        for (size_t i = 0; i < 16; i += 2) 
		{
            uint16_t segment = (addr.bytes[i] << 8) | addr.bytes[i + 1];
            oss << std::hex << std::setw(4) << std::setfill('0') << segment;
            if (i < 14) oss << ":";
        }

        return std::formatter<std::string>::format(oss.str(), ctx);
    }
};

struct AddressPair
{
	Address6 addr{};
	int32_t port{0};
	
	auto operator <=> (const AddressPair&) const = default;
};

template <>
struct std::hash<AddressPair>
{
  	std::size_t operator()(const AddressPair& k) const
	{
    	return ((std::hash<uint64_t>()(k.addr.tail) ^ (std::hash<int32_t>()(k.port) << 1)) >> 1);
	}
};

namespace IPv6Generator 
{
    Address6 generate();
};