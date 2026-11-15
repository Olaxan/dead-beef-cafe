#pragma once

#include <coroutine>
#include <mutex>
#include <memory>
#include <tuple>
#include <exception>
#include <utility>
#include <variant>

struct void_value {};

template<typename T>
using normalize_void =
    std::conditional_t<std::is_void_v<T>, void_value, T>;

template<typename... Results>
struct when_any_result 
{
    size_t index;
    std::variant<std::monostate, Results...> value;
};

template<typename A>
using await_result_t =
    decltype(std::declval<A>().await_resume());

template<typename... Awaitables>
class when_any_awaitable 
{
    using awaitables_type = std::tuple<Awaitables...>;

public:

    explicit when_any_awaitable(std::shared_ptr<awaitables_type> awaitables)
        : awaitables(std::move(awaitables)) {}

	template<typename... Results>
    struct shared_state 
	{
        std::mutex mutex;
        bool starting{true};
        bool completed{false};
        std::coroutine_handle<> continuation;
        std::exception_ptr exception;

        size_t winner = static_cast<size_t>(-1);
		std::variant<std::monostate, Results...> result;
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

        start_all(std::index_sequence_for<Awaitables...>{});

        bool resume = false;
        {
            std::lock_guard lock(state->mutex);
            state->starting = false;
            resume = state->completed;
        }

        if (resume)
            state->continuation.resume();
    }

    auto await_resume() 
	{
        if (state->exception)
            std::rethrow_exception(state->exception);

        return when_any_result<normalize_void<await_result_t<Awaitables>>...>
		{
			state->winner,
			std::move(state->result)
		};
    }

private:

    std::shared_ptr<awaitables_type> awaitables;
    std::shared_ptr<state_type> state;

    template<size_t... I>
    void start_all(std::index_sequence<I...>)
	{
        (start_one<I>(), ...);
    }

    template<size_t Index>
    void start_one()
	{
        run_one<Index>(awaitables, state);
    }

    template<size_t Index>
    static detached_task run_one(
        std::shared_ptr<awaitables_type> local_awaitables,
        std::shared_ptr<state_type> local_state)
	{
        try
		{
            auto value = co_await std::get<Index>(*local_awaitables);

            bool resume = false;
            {
                std::lock_guard lock(local_state->mutex);
                if (!local_state->completed)
                {
                    local_state->completed = true;
                    local_state->winner = Index;
                    local_state->result.template emplace<Index + 1>(std::move(value));
                    resume = !local_state->starting;
                }
            }

            if (resume)
                local_state->continuation.resume();
        }
        catch (...)
		{
            bool resume = false;
            {
                std::lock_guard lock(local_state->mutex);
                if (!local_state->completed)
                {
                    local_state->completed = true;
                    local_state->exception = std::current_exception();
                    local_state->winner = Index;
                    resume = !local_state->starting;
                }
            }

            if (resume)
                local_state->continuation.resume();
        }

        co_return;
	}
};

template<typename... Awaitables>
auto when_any(Awaitables&&... aw) 
{
    return when_any_awaitable<Awaitables...>(
        std::make_shared<std::tuple<Awaitables...>>(std::forward<Awaitables>(aw)...));
}