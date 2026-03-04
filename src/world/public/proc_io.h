#pragma once

#include <cstdint>
#include <memory>
#include <coroutine>
#include <print>

/* Awaiter for writing to a process. */
struct ProcessWriteAwaiter
{
	explicit ProcessWriteAwaiter() { }

    ProcessWriteAwaiter(ProcessWriteAwaiter&) = delete;

	bool await_ready()
	{
		return false;
	}

	void await_suspend(std::coroutine_handle<> h)
	{
        h.resume();
	}

	int32_t await_resume() const // returns: bytes sent
	{
		return next_.value();
	}

private:

	std::optional<int32_t> next_{};

};

/* Awaiter for reading from a process. */
template<typename T>
struct ProcessReadAwaiter
{
	explicit ProcessReadAwaiter() { }

    ProcessReadAwaiter(ProcessReadAwaiter&) = delete;

	bool await_ready()
	{
		return false;
	}

	void await_suspend(std::coroutine_handle<> h)
	{
        owner_->add_awaiter([this, h](const T& msg)
        {
            msg_ = msg;
            h.resume();
        });
	}

	T await_resume() const
	{
		return msg_.value();
	}

private:

	std::optional<T> msg_{};

};