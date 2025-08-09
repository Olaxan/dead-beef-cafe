#include "file.h"

void File::set_flag(FileModeFlags flag)
{
	flags_ = static_cast<FileModeFlags>(static_cast<uint8_t>(flags_) | static_cast<uint8_t>(flag));
}

void File::clear_flag(FileModeFlags flag)
{
	flags_ = static_cast<FileModeFlags>(static_cast<uint8_t>(flags_) & ~static_cast<uint8_t>(flag));
}

bool File::has_flag(FileModeFlags flag)
{
	return (static_cast<uint8_t>(flags_) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag);
}

void File::write(std::string content)
{
	content_ = std::move(content);
}

void File::append(std::string content)
{
	content_.append(std::move(content));
}

std::string_view File::get_view() const
{
	return std::string_view(content_);
}

std::stringstream File::get_stream() const
{
	return std::stringstream(content_);
}
