#include "file.h"

std::size_t File::size() const
{
	return content_.size();
}

std::size_t File::write(std::string content)
{
	content_ = std::move(content);
	return content_.size();
}

std::optional<std::string> File::read() const
{
	return content_;
}

std::optional<std::string> File::eat()
{
	return std::move(content_);
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

const std::string& File::get_string() const
{
	return content_;
}

std::optional<ArHandle> File::get_link() const
{
	if (link_.is_valid())
		return link_;
	
	return std::nullopt;
}
