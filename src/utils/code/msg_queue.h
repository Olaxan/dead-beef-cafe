#pragma once

#include <queue>
#include <mutex>
#include <string>
#include <optional>
#include <functional>
#include <ranges>

template<typename T>
class MessageQueue 
{
public:

    void push(T&& message) 
	{
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(std::move(message));
    }

    std::optional<T> pop() 
	{
        std::unique_lock<std::mutex> lock(mutex_);

        if (queue_.empty())
			return std::nullopt;

        T msg = queue_.front();
        queue_.pop_front();

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

    std::deque<T> copy()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return std::deque<T>(queue_);
    }

    std::deque<T> copy_clear()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        std::deque<T> copy = std::deque<T>(queue_);
        std::deque<T> empty{};
        std::swap(queue_, empty);
        return copy;
    }

private:

    std::deque<T> queue_{};
    std::mutex mutex_{};

};