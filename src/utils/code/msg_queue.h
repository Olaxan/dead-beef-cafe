#pragma once

#include <queue>
#include <mutex>
#include <string>
#include <optional>
#include <functional>

template<typename T>
using MessageNotifyFn = std::function<bool(const T&)>;

template<typename T>
class MessageQueue 
{
public:

    void push(T&& message) 
	{
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(message));

        bool received = false;
        for (auto& notify : notifies_)
            received = (received || std::invoke(notify, *queue_.back()));

        if (received)
        {
            pop();
            notifies_.clear();
        }
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

    void add_notify(MessageNotifyFn&& func)
    {
        notifies_.push_back(std::move(func));
    }

private:

    std::vector<MessageNotifyFn<T>> notifies_{};
    std::queue<T> queue_{};
    std::mutex mutex_{};

};