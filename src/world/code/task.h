#pragma once

#include <coroutine>
#include <optional>

#include <iostream>
#include <thread>

#include <chrono>
#include <queue>
#include <vector>

template<typename T, typename InitialSuspendT = std::suspend_always>
struct Task;

template<typename T, typename InitialSuspendT>
struct TaskPromiseType
{
    // value to be computed
    // when task is not completed (coroutine didn't co_return anything yet) value is empty
    std::optional<T> value;

    // corouine that awaiting this coroutine value
    // we need to store it in order to resume it later when value of this coroutine will be computed
    std::coroutine_handle<> awaiting_coroutine;

    // task is async result of our coroutine
    // it is created before execution of the coroutine body
    // it can be either co_awaited inside another coroutine
    // or used via special interface for extracting values (is_ready and get)
    Task<T, InitialSuspendT> get_return_object();

    InitialSuspendT initial_suspend() { return {}; }

    // store value to be returned to awaiting coroutine or accessed through 'get' function
    void return_value(T val)
    {
        value = std::move(val);
    }

    void unhandled_exception()
    {
        // alternatively we can store current exeption in std::exception_ptr to rethrow it later
        std::terminate();
    }

    // when final suspend is executed 'value' is already set
    // we need to suspend this coroutine in order to use value in other coroutine or through 'get' function
    // otherwise promise object would be destroyed (together with stored value) and one couldn't access task result
    // value
    auto final_suspend() noexcept
    {
        // if there is a coroutine that is awaiting on this coroutine resume it
        struct TransferAwaitable
        {
            std::coroutine_handle<> awaiting_coroutine;

            // always stop at final suspend
            bool await_ready() noexcept
            {
                return false;
            }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<TaskPromiseType> h) noexcept
            {
                // resume awaiting coroutine or if there is no coroutine to resume return special coroutine that do
                // nothing
                return awaiting_coroutine ? awaiting_coroutine : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        
        return TransferAwaitable{awaiting_coroutine};
    }

    // also we can await other task<T>
    template<typename U, typename InitialSuspendU>
    auto await_transform(Task<U, InitialSuspendU>& task)
    {
        if (!task.handle) {
            throw std::runtime_error("coroutine without promise awaited");
        }
        if (task.handle.promise().awaiting_coroutine) {
            throw std::runtime_error("coroutine already awaited");
        }

        struct TaskAwaitable
        {
            std::coroutine_handle<TaskPromiseType<U, InitialSuspendU>> handle;

            // check if this task already has value computed
            bool await_ready()
            {
                return handle.promise().value.has_value();
            }

            // h - is a handle to coroutine that calls co_await
            // store coroutine handle to be resumed after computing task value
            void await_suspend(std::coroutine_handle<> h)
            {
                handle.promise().awaiting_coroutine = h;
            }

            // when ready return value to a consumer
            auto await_resume()
            {
                return std::move(*(handle.promise().value));
            }
        };

        return TaskAwaitable{task.handle};
    }

    template<typename U>
    auto await_transform(U&& expr)
    {
        /* Leave this awaiter alone. */
        return std::forward<U>(expr);
    }

};

template<typename T, typename InitialSuspendT>
struct Task
{
    // declare promise type
    using promise_type = TaskPromiseType<T, InitialSuspendT>;

    Task(std::coroutine_handle<promise_type> handle) : handle(handle) {}

    Task(Task&& other) : handle(std::exchange(other.handle, nullptr)) {}

    Task& operator=(Task&& other)
    {
        if (handle) 
        {
            handle.destroy();
        }
        handle = std::exchange(other.handle, nullptr);
    }

    ~Task()
    {
        // if (handle) 
        // {
        //     handle.destroy();
        // }
    }

    /* Determine if the coroutine has finished. */
    bool is_done() const
    {
        if (handle)
            return handle.done();
        
        return true;
    }

    /* Interface for extracting value without awaiting on it. */
    bool is_ready() const
    {
        if (handle)
            return handle.promise().value.has_value();

        return false;
    }

    T get()
    {
        if (handle) {
            return std::move(*handle.promise().value);
        }
        throw std::runtime_error("get from task without promise");
    }

    std::coroutine_handle<promise_type> handle;
};

template<typename T, typename InitialSuspendT>
Task<T, InitialSuspendT> TaskPromiseType<T, InitialSuspendT>::get_return_object()
{
    return { std::coroutine_handle<TaskPromiseType>::from_promise(*this) };
}

template<typename T>
using LazyTask = Task<T, std::suspend_always>;

template<typename T>
using EagerTask = Task<T, std::suspend_never>;