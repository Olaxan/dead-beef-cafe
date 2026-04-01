#pragma once

#include <coroutine>
#include <atomic>
#include <tuple>
#include <exception>
#include <utility>
#include <variant>

template<typename A>
using await_result_t =
    decltype(std::declval<A>().await_resume());

template<typename Awaitable>
class when_all_dynamic_awaitable 
{

    struct void_value {};

    template<typename T>
    using normalize_void =
        std::conditional_t<std::is_void_v<T>, void_value, T>;

    using results_type = 
        normalize_void<await_result_t<Awaitable>>;

public:

    explicit when_all_dynamic_awaitable(std::vector<Awaitable>&& aw)
        : awaitables(std::move(aw)) {}

    struct shared_state 
	{
        std::atomic<int32_t> num_remaining{0};
        std::coroutine_handle<> continuation;
        std::exception_ptr exception;

        std::vector<results_type> results;
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
        state->num_remaining = awaitables.size();
        state->results.resize(state->num_remaining);

        start_all();
    }

    auto await_resume() 
	{
        if (state->exception)
            std::rethrow_exception(state->exception);

        return std::move(state->results);
    }

private:

    std::vector<Awaitable> awaitables;
    std::shared_ptr<shared_state> state;

    void start_all()
	{
        for (std::size_t i = 0; i < awaitables.size(); ++i)
        {
            run_one(awaitables.at(i), i);
        }
    }

    void run_one(Awaitable& aw, std::size_t index) 
	{
        auto runner = [&aw](std::shared_ptr<shared_state> local_state, std::size_t index) -> Task<bool>
		{
            try 
			{
                auto value = co_await aw;

                local_state->results[index] = std::move(value);

                if (--local_state->num_remaining == 0)
                {
                    local_state->continuation.resume();
                }
            } 
			catch (...) 
			{
                if (--local_state->num_remaining == 0)
                {
                    local_state->exception = std::current_exception();
                    local_state->continuation.resume();
                }
            }

            co_return true;
        };

        runner(state, index);
    }
};

template<typename Awaitable>
auto when_all_dynamic(std::vector<Awaitable>&& aw) 
{
    return when_all_dynamic_awaitable<Awaitable>(std::move(aw));
}