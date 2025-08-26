#pragma once

#include "os.h"
#include "addr.h"
#include "proc.h"

namespace NetUtils
{
	// Socket create_socket(Proc& proc);
	// bool bind_socket(Proc& proc, Socket sock, int32_t port);
	
	// AcceptAwaiter accept(Proc& proc, Socket sock);
	
	// bool connect(Proc& proc, Socket sock, AddressPair dest);
	// bool connect(Proc& proc, Socket sock, Address6 addr, int32_t port);

	// WriteAwaiter async_write(Socket sock, std::string_view str);
	// ReadAwaiter async_read(Socket sock);
}