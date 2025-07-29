#pragma once

#include "msg_queue.h"

#include <coroutine>


using SocketStreamFn = std::function<void(void)>;

template<typename T>
struct MessageAwaiter;

struct Address6
{
	Address6(int64_t bytes)
		: internal(bytes) {}

	Address6(std::string addr)
		: internal(0) {}

	int64_t internal{0};

	auto operator <=> (const Address6&) const = default;
};

struct AddressPair
{
	Address6 addr{0};
	int32_t port{0};
	
	auto operator <=> (const AddressPair&) const = default;
};

template <>
struct std::hash<AddressPair>
{
  	std::size_t operator()(const AddressPair& k) const
	{
    	return ((std::hash<int64_t>()(k.addr.internal) ^ (std::hash<int32_t>()(k.port) << 1)) >> 1);
	}
};


class ISocket 
{
public:
	virtual SocketStreamFn open_stream(ISocket* other) = 0;
};

template<typename T_Rx, typename T_Tx>
class Socket : public ISocket, public std::enable_shared_from_this<Socket<T_Rx, T_Tx>>
{
public:

	Socket() = default;

	auto read_one()
	{
		return MessageAwaiter<T_Rx>(rxq);
	}
	
	bool write_one(T_Tx&& msg)
	{
		txq.push(std::move(msg));
		return true;
	}

	bool is_open() const { return false; }

	virtual SocketStreamFn open_stream(ISocket* other) override;
	
	MessageQueue<T_Rx> rxq{};
	MessageQueue<T_Tx> txq{};

private:


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
		// do nothing for now
	}

	T await_resume() const
	{
		return next_.value();
	}

private:

	std::optional<T> next_{};
	MessageQueue<T>& queue_;

};

template <typename T_Rx, typename T_Tx>
inline SocketStreamFn Socket<T_Rx, T_Tx>::open_stream(ISocket* other)
{
	using LocalEndpoint = Socket<T_Rx, T_Tx>;
	using RemoteEndpoint = Socket<T_Tx, T_Rx>;

	LocalEndpoint* local_raw = dynamic_cast<LocalEndpoint*>(this);
	RemoteEndpoint* remote_raw = dynamic_cast<RemoteEndpoint*>(other);

	if (local_raw && remote_raw)
	{
		return [local = local_raw->shared_from_this(), remote = remote_raw->shared_from_this()]()
		{
			if (std::optional<T_Rx> msg = remote->txq.pop(); msg.has_value())
			{
				local->rxq.push(std::move(msg.value()));
			}

			if (std::optional<T_Tx> msg = local->txq.pop(); msg.has_value())
			{
				remote->rxq.push(std::move(msg.value()));
			}
		};
	}

	return nullptr;
}
