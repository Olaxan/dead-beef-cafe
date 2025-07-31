#include "os.h"
#include "host.h"
#include "world.h"
#include "proc.h"
#include "ip_mgr.h"
#include "nic.h"

#include <sstream>
#include <string>
#include <coroutine>
#include <chrono>
#include <iostream>
#include <print>

std::size_t OS::register_devices()
{
    int32_t uuid = 0;
    devices_.clear();
    for (auto& dev : owner_.get_devices())
        devices_.emplace(++uuid, dev.get());
    
    return devices_.size();
}

void OS::shutdown_os()
{
    owner_.shutdown_host();
}

const std::string& OS::get_hostname() const
{
	return owner_.get_hostname();
}

World& OS::get_world()
{
    return owner_.get_world();
}

Proc* OS::get_shell(std::ostream& out_stream)
{
    Proc* proc = create_process(out_stream);

    if (proc == nullptr)
    {
        std::println(out_stream, "Failed to create shell process.");
        return nullptr;
    }

    auto sh = get_default_shell();
    proc->dispatch(sh, {});

    return proc;
}

Proc* OS::create_process(std::ostream& os)
{
    /* Here, we create the process object, which will hold data for the running process. 
    The process function itself, however, is not run yet. */
    int32_t pid = pid_counter_++;
    std::println("Created process (pid = {0}) on {1}.", pid, (int64_t)this);
    auto [it, success] = processes_.emplace(std::make_pair(pid, Proc{pid, this, os}));

    return success ? &(it->second) : nullptr;
}

Proc* OS::create_process(Proc* host)
{
    int32_t pid = pid_counter_++;
    std::println("Created child process (pid = {0}) under {1} on {2}.", pid, host->pid, (int64_t)this);
    auto [it, success] = processes_.emplace(std::make_pair(pid, Proc{pid, host}));

    return success ? &(it->second) : nullptr;
}

EagerTask<int32_t> OS::create_process(ProcessFn program, std::vector<std::string> args, std::ostream &os)
{
    Proc* proc = create_process(os);
    int32_t pid = proc->get_pid();

    if (proc == nullptr)
        co_return 1;

    /* Now we actually run (and await) the process. */
    int32_t ret = co_await proc->await_dispatch(program, std::move(args));
    
    //std::println("Task {0} finished with code {1}.", pid, ret);
    
    processes_.erase(pid); // Use pid instead of iterator in case it's been invalidated after awaiting.
    co_return ret;
}

EagerTask<int32_t> OS::create_process(ProcessFn program, std::vector<std::string> args, Proc* host)
{
	Proc* proc = create_process(host);
    int32_t pid = proc->get_pid();

    if (proc == nullptr)
        co_return 1;

    int32_t ret = co_await proc->await_dispatch(program, std::move(args));
    
    processes_.erase(pid);
    co_return ret;
}

void OS::get_processes(std::function<void(const Proc&)> reader) const
{
    for (const auto& [pid, proc] : processes_)
        std::invoke(reader, proc);
}

Address6 OS::get_global_ip()
{
	if (NIC* nic = get_device<NIC>())
        return nic->get_ip();

    return Address6();
}

TimerAwaiter OS::wait(float seconds)
{
	return get_world().get_timer_manager().wait(seconds);
}

void OS::schedule(float seconds, schedule_fn callback)
{
    get_world().get_timer_manager().set_timer(seconds, callback);
}
