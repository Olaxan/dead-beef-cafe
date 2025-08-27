#pragma once

#include <queue>
#include <mutex>
#include <string>
#include <optional>
#include <functional>
#include <coroutine>
#include <ranges>

template<typename T>
using MessageCallbackFn = std::function<void(const T&)>;

template<typename T>
using MessageCallbackList = std::vector<MessageCallbackFn<T>>;

template<typename T>
struct MessageQueueAwaiter;

template<typename T>
class MessageQueue 
{
public:

    MessageQueue() = default;

    ~MessageQueue()
    {
        broadcast_clear(T{});
    }

    void push(T&& message) 
	{
        /* Scoped lock */
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (callbacks_.empty())
            {
                queue_.emplace_back(std::move(message));
                return;
            }
        }
        
        broadcast_clear(std::move(message));
    }

    void push(const T& message) 
	{
        /* Scoped lock */
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (callbacks_.empty())
            {
                queue_.emplace_back(message);
                return;
            }
        }

        T copy = message;
        broadcast_clear(std::move(copy));
    }

    std::optional<T> pop() 
	{
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty())
			return std::nullopt;

        T msg = queue_.front();
        queue_.pop_front();

		return msg;
    }

    std::optional<T> peek() 
	{
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty())
			return std::nullopt;

        return queue_.front();
    }

    MessageQueueAwaiter<T> async_read()
    {
        return MessageQueueAwaiter<T>(this);
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    std::deque<T> copy()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::deque<T>(queue_);
    }

    std::deque<T> copy_clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::deque<T> copy = std::deque<T>(queue_);
        std::deque<T> empty{};
        std::swap(queue_, empty);
        return copy;
    }

    void add_awaiter(MessageCallbackFn<T>&& callback)
    {
        callbacks_.push_back(std::move(callback));
    }

    void broadcast_clear(const T&& message)
    {
        MessageCallbackList<T> empty{};
        T local_message = std::move(message);
        {
            /* In this critical section, we swap the callbacks array
            to the empty one, before we actually iterate over anything.
            When we iterate, the mutex is unlocked, as we now own all
            resources locally within this stack. */
            
            std::scoped_lock<std::mutex> lock(mutex_);
            std::swap(callbacks_, empty);
        }
        for (auto& fn : empty) { fn(local_message); }
    }

private:

    std::deque<T> queue_{};
    std::mutex mutex_{};
    MessageCallbackList<T> callbacks_{};

};


template<typename T>
struct MessageQueueAwaiter
{
	using OwningQueue = MessageQueue<T>;

	explicit MessageQueueAwaiter(OwningQueue* owner)
		: owner_(owner) { }

	bool await_ready()
	{
		next_ = owner_->pop();
		return next_.has_value();
	}

	void await_suspend(std::coroutine_handle<> h)
	{
        owner_->add_awaiter([this, h](const T& msg)
        {
            next_ = msg;
            h.resume();
        });
	}

	T await_resume() const
	{
		return next_.value();
	}

private:

    OwningQueue* owner_{nullptr};
	std::optional<T> next_{};

};