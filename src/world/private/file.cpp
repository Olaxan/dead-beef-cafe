#include "file.h"

std::size_t File::size() const
{
	return content_.size();
}

void File::write(ProcessFn exec)
{
	executable_ = std::move(exec);
	content_ = "BIN64::";
	notify_write();
}

// void File::write(ProcessFn&& exec)
// {
// 	executable_ = exec;
// 	content_ = "BIN64::";
// }

void File::write(std::string content)
{
	content_ = std::move(content);
	notify_write();
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
	notify_write();
}

std::string_view File::get_view() const
{
	return std::string_view(content_);
}

std::stringstream File::get_stream() const
{
	return std::stringstream(content_);
}

const ProcessFn& File::get_executable() const
{
	return executable_;
}

void File::notify_write()
{
	std::vector<FileWriteCallbackFn> copy = callbacks_;

	for (auto&& callback : copy)
		callback(content_);
}
