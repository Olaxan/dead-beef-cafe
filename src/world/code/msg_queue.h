#pragma once

#include <queue>
#include <mutex>
#include <string>
#include <optional>

class MessageQueue 
{
public:

    void push(const std::string& message) 
	{
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(message);
    }

    std::optional<std::string> pop() 
	{
        std::unique_lock<std::mutex> lock(mutex_);

		if (queue_.empty())
			return std::nullopt;

		std::string msg = queue_.front();
		queue_.pop();
		return msg;
    }

private:

    std::queue<std::string> queue_;
    std::mutex mutex_;

};