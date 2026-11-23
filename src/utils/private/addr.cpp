#include "addr.h"

#include <random>

Address6::Address6(std::array<uint8_t, 16>&& raw_bytes) noexcept
{
	bytes = std::move(raw_bytes);
}

std::expected<Address6, std::invalid_argument> Address6::from_string(const std::string& address) noexcept
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
			return std::unexpected(std::invalid_argument("Too many octets in IPv6 address"));

		parts.insert(parts.begin() + head_count, missing, "0");
	} 
	else 
	{
		split(address, parts);
		if (parts.size() != 8)
			return std::unexpected(std::invalid_argument("Invalid IPv6 address format"));
	}

	std::array<uint8_t, 16> out_bytes{0};
	for (size_t i = 0; i < parts.size(); ++i) 
	{
		try
		{
			uint16_t value = std::stoi(parts[i], nullptr, 16);
			out_bytes[i * 2] = static_cast<uint8_t>(value >> 8);
			out_bytes[i * 2 + 1] = static_cast<uint8_t>(value & 0xFF);
		}
		catch (std::out_of_range e)
		{
			return std::unexpected(std::invalid_argument("Octet member out of range"));
		}
		catch (std::invalid_argument e)
		{
			return std::unexpected(e);
		}
	}

	return Address6(std::move(out_bytes));
}

std::expected<Address6, std::invalid_argument> Address6::from_bytes(std::string_view str) noexcept
{
	if (str.size() < 16)
	{
		return std::unexpected(std::invalid_argument(
			std::format("Address6 bytes constructor takes 16 bytes, got {}", str.size())));
	}

	std::array<uint8_t, 16> out_bytes{0};
	std::memcpy(out_bytes.data(), str.data(), 16);
	return Address6(std::move(out_bytes));
}

std::expected<Address6, std::invalid_argument> Address6::mask_subnet(Address6 address, uint8_t prefix) noexcept
{
	if (prefix > 128)
		return std::unexpected(std::invalid_argument("IPv6 network prefix exceeds 128 bits."));

	for (std::size_t bit = prefix; bit < 128; ++bit)
	{
		address.bytes[bit / 8] &= static_cast<uint8_t>(~(1u << (7 - bit % 8)));
	}
	return address;
}

std::expected<Address6, std::invalid_argument> Address6::allocate_subnet(Address6 address_space, uint8_t address_space_prefix, uint8_t allocation_prefix, std::size_t index) noexcept
{
	if (address_space_prefix >= allocation_prefix || allocation_prefix > 128)
		return std::unexpected(std::invalid_argument("Child network prefix must be longer than its address space prefix."));

	const std::size_t index_bits = allocation_prefix - address_space_prefix;
	if (index_bits < std::numeric_limits<std::size_t>::digits &&
		index >= (std::size_t{1} << index_bits))
	{
		return std::unexpected(std::invalid_argument("Generation plan exceeds the available IPv6 subnets."));
	}

	return mask_subnet(address_space, allocation_prefix)
	.transform([&](Address6 subnet)
	{
		for (std::size_t bit = 0; bit < index_bits; ++bit)
		{
			const std::size_t index_bit = index_bits - bit - 1;
			if (index_bit < std::numeric_limits<std::size_t>::digits &&
				((index >> index_bit) & 1u) != 0)
			{
				subnet.bytes[(address_space_prefix + bit) / 8] |=
					static_cast<uint8_t>(1u << (7 - (address_space_prefix + bit) % 8));
			}
		}

		return subnet;
	});
}

std::expected<Address6, std::overflow_error> Address6::add_address_offset(Address6 address, std::size_t offset) noexcept
{
	std::size_t carry = offset;
	for (std::size_t i = address.bytes.size(); i > 0 && carry > 0; --i)
	{
		const std::size_t sum = address.bytes[i - 1] + (carry & 0xFFu);
		address.bytes[i - 1] = static_cast<uint8_t>(sum & 0xFFu);
		carry = (carry >> 8) + (sum >> 8);
	}

	if (carry != 0)
		return std::unexpected(std::overflow_error("Generated IPv6 host address overflowed."));

	return address;
}

void Address6::split(const std::string& str, std::vector<std::string>& out)
{
	std::stringstream ss(str);
	std::string segment;
	while (std::getline(ss, segment, ':')) 
	{
		if (!segment.empty())
			out.push_back(segment);
	}
}

Address6 IPv6Generator::generate()
{
	Address6 out;
	std::array<uint8_t, 16>& bytes = out.bytes;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 255);

	for (auto& byte : bytes) {
		byte = static_cast<uint8_t>(dis(gen));
	}

	return out;
}