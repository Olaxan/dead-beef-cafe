#include "file.h"

std::string_view File::get_view() const
{
	return std::string_view(content_);
}

std::stringstream File::get_stream() const
{
	return std::stringstream(content_);
}
