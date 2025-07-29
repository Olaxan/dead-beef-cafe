#pragma once

#include <queue>
#include <mutex>
#include <string>
#include <optional>
#include <functional>

template<typename T>
class MessageQueue 
{
public:

    void push(T&& message) 
	{
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(message));
    }

    std::optional<T> pop() 
	{
        std::unique_lock<std::mutex> lock(mutex_);

        if (queue_.empty())
			return std::nullopt;

        T msg = queue_.front();
	    queue_.pop();

		return msg;
    }

    std::optional<T> peek() 
	{
        std::unique_lock<std::mutex> lock(mutex_);

        if (queue_.empty())
			return std::nullopt;

        return queue_.front();
    }

    bool empty() const
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:

    std::queue<T> queue_{};
    std::mutex mutex_{};

};