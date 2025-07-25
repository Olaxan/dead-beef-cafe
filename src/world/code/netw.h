#pragma once

#include "msg_queue.h"

class OS;

template<typename T>
struct MessageAwaiter;

class ISocket { };

template<typename T_Rx, typename T_Tx>
class Socket : public ISocket
{
public:

	Socket() = delete;
	Socket(int32_t port, OS* owner)
		: port_(port), owner_(owner) { }

	auto read_one()
	{
		return MessageAwaiter<T_Rx>(rxq);
	}
	
	bool write_one(T_Tx&& msg)
	{
		txq.push(std::move(msg));
	}

	bool is_open() const { return false; }

	bool connect() { return false; }

private:

	MessageQueue<T_Rx> rxq{};
	MessageQueue<T_Tx> txq{};
	int32_t port_{0};
	OS* owner_{nullptr};

};

template<typename T_RxTx>
using SymSocket = Socket<T_RxTx, T_RxTx>;

/* Awaiter */
template<typename T>
struct MessageAwaiter
{
	explicit MessageAwaiter(MessageQueue<T>& queue)
		: queue_(queue) { }

	bool await_ready()
	{
		next_ = queue_.pop();
		return next_.has_value();
	}

	void await_suspend(std::coroutine_handle<> h)
	{
		queue_.add_notify([this, h](const T& msg) 
		{
			next_ = msg;
			h.resume();
			return true;
		});
	}

	T await_resume() const
	{
		return next_.value();
	}

private:

	std::optional<T> next_{};
	MessageQueue<T>& queue_;

};