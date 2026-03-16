#pragma once

#include <coroutine>
#include <atomic>
#include <tuple>
#include <exception>
#include <utility>

template<typename... Awaitables>
class when_any_awaitable {
public:
    explicit when_any_awaitable(Awaitables&&... aw)
        : awaitables(std::forward<Awaitables>(aw)...) {}

    struct shared_state 
	{
        std::atomic<bool> completed{false};
        std::coroutine_handle<> continuation;
        std::exception_ptr exception;
        size_t winner = static_cast<size_t>(-1);
    };

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
        state = std::make_shared<shared_state>();
        state->continuation = h;

        start_all(std::index_sequence_for<Awaitables...>{});
    }

    size_t await_resume() 
	{
        if (state->exception)
            std::rethrow_exception(state->exception);

        return state->winner;
    }

private:

    std::tuple<Awaitables...> awaitables;
    std::shared_ptr<shared_state> state;

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
        auto state_copy = state;

        auto runner = [state_copy, &aw]() -> detached_task
		{
            try 
			{
                co_await aw;

                if (!state_copy->completed.exchange(true))
				{
                    state_copy->winner = Index;
                    state_copy->continuation.resume();
                }

            } 
			catch (...) 
			{

                if (!state_copy->completed.exchange(true))
				{
                    state_copy->exception = std::current_exception();
                    state_copy->winner = Index;
                    state_copy->continuation.resume();
                }
            }

            co_return;
        };

        runner();
    }
};

template<typename... Awaitables>
auto when_any(Awaitables&&... aw) 
{
    return when_any_awaitable<Awaitables...>(std::forward<Awaitables>(aw)...);
}