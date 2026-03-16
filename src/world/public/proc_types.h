#pragma once

#include "task.h"
#include <functional>
#include <coroutine>
#include <system_error>
#include <expected>

class Proc;

using ProcessTask = Task<int32_t, std::suspend_always>;
using ProcessFn = std::function<ProcessTask(Proc&, std::vector<std::string>)>;

using ReadResult = std::expected<std::string, std::error_condition>;

using WriterFn = std::move_only_function<void(const std::string&)>;
using ReaderFn = std::move_only_function<Task<ReadResult>(void)>;
using InvokeFn = std::function<void(Proc*)>;

template <class... Ts> struct WriterOverload : Ts... { using Ts::operator()...; };
template <class... Ts> WriterOverload(Ts...) -> WriterOverload<Ts...>;

using SignalType = int32_t;
using SignalCallbackFn = std::function<void(SignalType)>;