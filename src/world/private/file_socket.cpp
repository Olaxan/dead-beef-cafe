#include "file_socket.h"

#include <print>

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

std::optional<std::string> SocketFile::read_rx()
{
	return rx_queue.pop();
}

std::optional<std::string> SocketFile::read_tx()
{
	return tx_queue.pop();
}
