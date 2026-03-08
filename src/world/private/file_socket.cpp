#include "file_socket.h"

void SocketFile::write(std::string content)
{
	tx_queue.push(std::move(content));
}

std::optional<std::string> SocketFile::read() const
{
	return rx_queue.peek();
}

std::optional<std::string> SocketFile::eat()
{
	return rx_queue.pop();
}