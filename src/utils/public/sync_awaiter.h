#pragma once

#include <coroutine>
#include <atomic>
#include <tuple>
#include <exception>
#include <utility>
#include <variant>

struct void_value {};

template<typename... Results>
using when_all_result =
    std::tuple<std::monostate, Results...>;

template<typename A>
using await_result_t =
    decltype(std::declval<A>().await_resume());

template<typename... Awaitables>
class when_all_awaitable 
{

    template<typename T>
    using normalize_void =
        std::conditional_t<std::is_void_v<T>, void_value, T>;

public:

    explicit when_all_awaitable(Awaitables&&... aw)
        : awaitables(std::forward<Awaitables>(aw)...) {}

	template<typename... Results>
    struct shared_state 
	{
        std::atomic<int32_t> num_remaining{0};
        std::coroutine_handle<> continuation;
        std::exception_ptr exception;

		when_all_result<Results...> results;
    };

	using state_type =
    	shared_state<normalize_void<await_result_t<Awaitables>>...>;

	struct detached_task
	{
		struct promise_type
		{
			detached_task get_return_object()
			{
				return {};
			}

			std::suspend_never initial_suspend() noexcept { return {}; }
			std::suspend_never final_suspend() noexcept { return {}; }

			void return_void() noexcept {}
			void unhandled_exception() { std::terminate(); }
		};
	};

    bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) 
	{
        state = std::make_shared<state_type>();
        state->continuation = h;
        state->num_remaining = std::tuple_size<decltype(awaitables)>();

        start_all(std::index_sequence_for<Awaitables...>{});
    }

    auto await_resume() 
	{
        if (state->exception)
            std::rethrow_exception(state->exception);

        return std::move(state->results);
    }

private:

    std::tuple<Awaitables...> awaitables;
    std::shared_ptr<state_type> state;

    template<size_t... I>
    void start_all(std::index_sequence<I...>)
	{
        (start_one<I>(), ...);
    }

    template<size_t Index>
    void start_one()
	{
        auto& aw = std::get<Index>(awaitables);
        run_one<Index>(aw);
    }

    template<size_t Index, typename Awaitable>
    void run_one(Awaitable& aw) 
	{
        auto runner = [&aw](std::shared_ptr<state_type> local_state) -> Task<bool>
		{
            auto state_copy = std::move(local_state);

            try 
			{
                auto value = co_await aw;

                std::get<Index + 1>(state_copy->results) = std::move(value);

                if (--state_copy->num_remaining == 0)
                {
                    state_copy->continuation.resume();
                }
            } 
			catch (...) 
			{
                if (--state_copy->num_remaining == 0)
                {
                    state_copy->exception = std::current_exception();
                    state_copy->continuation.resume();
                }
            }

            co_return true;
        };

        runner(state);
    }
};

template<typename... Awaitables>
auto when_all(Awaitables&&... aw) 
{
    return when_all_awaitable<Awaitables...>(std::forward<Awaitables>(aw)...);
}