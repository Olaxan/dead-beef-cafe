#pragma once

#include "file.h"

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
struct ProcessReadAwaiter
{
	ProcessReadAwaiter(std::shared_ptr<File> file = nullptr)
		: file_(file) { }

    ProcessReadAwaiter(ProcessReadAwaiter&) = delete;
	ProcessReadAwaiter(ProcessReadAwaiter&&) = default;
	~ProcessReadAwaiter() = default;

	bool await_ready()
	{
		/* If the file is not valid, we don't want to yield but instead immediately return empty. */
		if (file_ == nullptr)
			return true;

		/* If the file contains some data already, we can return immediately. */
		if (file_->size() > 0)
			return true;

		return false;
	}

	void await_suspend(std::coroutine_handle<> h)
	{
        file_->add_read_callback([h](std::string_view content)
		{
			h.resume();
		});
	}

	std::string await_resume() const
	{
		if (file_ == nullptr)
			return {};

		return file_->eat().value_or("");
	}

private:


	std::shared_ptr<File> file_{nullptr};

};