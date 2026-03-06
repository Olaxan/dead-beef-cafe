#pragma once

#include "task.h"
#include <functional>
#include <coroutine>

class Proc;

using ProcessTask = Task<int32_t, std::suspend_always>;
using ProcessFn = std::function<ProcessTask(Proc&, std::vector<std::string>)>;

template <class... Ts> struct WriterOverload : Ts... { using Ts::operator()...; };
template <class... Ts> WriterOverload(Ts...) -> WriterOverload<Ts...>;