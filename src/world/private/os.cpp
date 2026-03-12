#include "os.h"
#include "host.h"
#include "world.h"
#include "proc.h"
#include "net_srv.h"
#include "nic.h"
#include "disk.h"

#include <sstream>
#include <string>
#include <coroutine>
#include <chrono>
#include <iostream>
#include <print>
#include <unordered_set>

OS::OS(Host& owner)
    : owner_(owner) 
{ 
    register_devices(); 
}

OS::~OS()
{
}

std::size_t OS::register_devices()
{
    int32_t uuid = 0;
    devices_.clear();
    for (auto& dev : owner_.get_devices())
        devices_.emplace(++uuid, dev.get());
    
    return devices_.size();
}

void OS::start_os()
{
    state_ = DeviceState::PoweredOn;
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

Host& OS::get_owner()
{
	return owner_;
}

Proc* OS::create_process(CreateProcessParams&& params)
{
    /* Here, we create the process object, which will hold data for the running process. 
    The process function itself, however, is not run yet. */
    int32_t pid = ++pid_counter_;

    auto [iproc, success] = processes_.emplace(std::make_pair(pid, std::make_unique<Proc>(pid, this)));

    if (!success)
        return nullptr;

    Proc* proc = iproc->second.get();

    proc->set_uid(params.uid);
    proc->set_gid(params.gid);

    if (params.invoke)
    {
        params.invoke(proc);
    }

    /* The invoker might have set up a writer,
    if not: check if one is set up in params,
    if not: use stdout. */
    if (proc->writer_ == nullptr)
    {
        if (params.writer)
        {
            proc->set_writer(std::move(params.writer));
        }
        else
        {
            proc->set_writer([pid](const std::string& str)
            {
                std::cout << str;
            });
        }
    }

    /* The invoker might have setup the reader,
    if not, check if one is assigned to the params. */
    if (proc->reader_ == nullptr)
    {
        if (params.reader)
        {
            proc->set_reader(std::move(params.reader));
        }
    }

    if (auto ihost = processes_.find(params.leader_id); ihost != processes_.end())
    {
        Proc* leader = ihost->second.get();
        proc->set_leader(leader);
        std::println("Created process {} (uid {}, gid {}) under {} on {}.", 
            pid, proc->get_uid(), proc->get_gid(), ihost->first, (int64_t)this);
    }
    else
    {
        std::println("Created process {} (uid {}, gid {}) on {}.", 
            pid, proc->get_uid(), proc->get_gid(), (int64_t)this);
    }

    return proc;
}

EagerTask<int32_t> OS::run_process(ProcessFn program, std::vector<std::string> args, CreateProcessParams&& params)
{
    Proc* proc = create_process(std::forward<CreateProcessParams>(params));
    int32_t pid = proc->get_pid();

    if (proc == nullptr)
        co_return 1;

    /* Now we actually run (and await) the process. */
    int32_t ret = co_await proc->await_dispatch(program, std::move(args));

    proc->exit();
    processes_.erase(pid);
    co_return ret;
}

int32_t OS::create_sid()
{
    return ++pid_counter_;
}

void OS::get_processes(std::function<void(const Proc&)> reader) const
{
    for (const auto& [pid, proc] : processes_)
        std::invoke(reader, *proc);
}

FileSystem* OS::get_filesystem() const
{
	if (Disk* disk = get_device<Disk>())
        return disk->get_fs();

    return nullptr;
}

UsersManager* OS::get_users_manager()
{
	return &users_;
}

SessionManager* OS::get_session_manager()
{
	return &sess_;
}

NetManager* OS::get_network_manager()
{
	return &net_;
}

TimerAwaiter OS::wait(float seconds)
{
	return get_world().get_timer_manager().wait(seconds);
}

void OS::schedule(float seconds, SchedulerFn callback)
{
    get_world().get_timer_manager().set_timer(seconds, callback);
}
