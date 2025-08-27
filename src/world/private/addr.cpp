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
		uint16_t value = std::stoi(parts[i], nullptr, 16);
		out_bytes[i * 2] = static_cast<uint8_t>(value >> 8);
		out_bytes[i * 2 + 1] = static_cast<uint8_t>(value & 0xFF);
	}

	return Address6(std::move(out_bytes));
}

std::expected<Address6, std::invalid_argument> Address6::from_bytes(std::string_view str) noexcept
{
	if (str.size() != 16)
		return std::unexpected(std::invalid_argument("Address6 bytes constructor takes 16 bytes."));

	std::array<uint8_t, 16> out_bytes{0};
	std::memcpy(out_bytes.data(), str.data(), 16);
	return Address6(std::move(out_bytes));
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