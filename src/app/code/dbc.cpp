#include "dbc.h"

#include <asio.hpp>

std::string DbcUtils::make_string(asio::streambuf& streambuf)
{
	return { asio::buffers_begin(streambuf.data()), asio::buffers_end(streambuf.data()) };
}
