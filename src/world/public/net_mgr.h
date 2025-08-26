#pragma once

#include "addr.h"
#include "str_sock.h"

#include <string>
#include <expected>
#include <unordered_map>

struct SocketDescriptor
{
	int32_t fd{0};
};

class OS;
class NetManager
{
public:

	NetManager() = delete;
	NetManager(OS* owner) : os_(*owner) { }


	/* TODO: Don't use string. */
	std::expected<SocketDescriptor, std::string> create_socket();

	int32_t bind_socket(SocketDescriptor sock, AddressPair addr);


protected:

	OS& os_;

	std::unordered_map<int32_t, StringSocket> sockets_;
	std::unordered_map<AddressPair, int32_t> bindings_;

};