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
#include <concepts>

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

	template<std::unsigned_integral T>
		requires (!std::same_as<T, bool> && sizeof(T) <= 16)
	Address6(T value) noexcept
		: bytes{}
	{
		for (size_t i = 0; i < sizeof(T); ++i)
		{
			bytes[sizeof(T) - i - 1] = static_cast<uint8_t>(value & static_cast<T>(0xFF));
			value >>= 8;
		}
	}

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

    std::string_view data_view() const
    {
        return std::string_view{raw, 16};
    }

    std::string data_str() const
    {
        return std::string{raw, 16};
    }

    bool is_valid() const
    {
        return (head > 0) || (tail > 0);
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
        std::array<uint16_t, 8> segments{};
        for (size_t i = 0; i < segments.size(); ++i)
        {
            segments[i] = static_cast<uint16_t>(
                (static_cast<uint16_t>(addr.bytes[i * 2]) << 8) | addr.bytes[i * 2 + 1]);
        }

        size_t best_start = segments.size();
        size_t best_length = 0;
        for (size_t i = 0; i < segments.size();)
        {
            if (segments[i] != 0)
            {
                ++i;
                continue;
            }

            size_t run_start = i;
            while (i < segments.size() && segments[i] == 0)
                ++i;

            size_t run_length = i - run_start;
            if (run_length >= 2 && run_length > best_length)
            {
                best_start = run_start;
                best_length = run_length;
            }
        }

        std::ostringstream oss;
        bool has_output = false;
        bool after_compression = false;
        for (size_t i = 0; i < segments.size(); ++i)
		{
            if (i == best_start)
            {
                oss << "::";
                has_output = true;
                after_compression = true;
                i += best_length - 1;
                continue;
            }

            if (has_output && !after_compression)
                oss << ":";
            oss << std::hex << segments[i];
            has_output = true;
            after_compression = false;
        }

        return std::formatter<std::string>::format(oss.str(), ctx);
    }
};

template <>
struct std::hash<Address6>
{
  	std::size_t operator()(const Address6& k) const
	{
    	return std::hash<uint64_t>()(k.tail);
	}
};


struct AddressPair
{
	Address6 addr{};
	int32_t port{0};
	
	auto operator <=> (const AddressPair&) const = default;
};

struct AddressTuple
{
    AddressTuple() = default;
    
    AddressTuple(AddressPair src, AddressPair dest)
        : source(std::move(src)), dest(std::move(dest)) {}

    AddressTuple(Address6 src_addr, int32_t src_port, Address6 dest_addr, int32_t dest_port)
        : source({src_addr, src_port}), dest({src_addr, src_port}) {}

    AddressPair source{};
    AddressPair dest{};

    auto operator <=> (const AddressTuple&) const = default;
};

template <>
struct std::hash<AddressPair>
{
  	std::size_t operator()(const AddressPair& k) const
	{
    	return ((std::hash<uint64_t>()(k.addr.tail) ^ (std::hash<int32_t>()(k.port) << 1)) >> 1);
	}
};

template <>
struct std::hash<AddressTuple>
{
  	std::size_t operator()(const AddressTuple& k) const
	{
    	return ((std::hash<AddressPair>()(k.source) ^ (std::hash<AddressPair>()(k.dest) << 1)) >> 1);
	}
};

namespace IPv6Generator 
{
    Address6 generate();
};