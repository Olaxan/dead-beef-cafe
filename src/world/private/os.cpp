#include "os.h"
#include "host.h"
#include "world.h"
#include "proc.h"
#include "net_srv.h"
#include "nic.h"
#include "disk.h"

#include "proto/host.pb.h"
#include "proto/files.pb.h"

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

    /* Run invoker, if set. */
    if (params.invoke)
    {
        params.invoke(proc);
    }

    /* Setup writer, if none is set (eg. by invoker). */
    if (proc->writer_ == nullptr && params.writer)
    {
        proc->set_writer(std::move(params.writer));
    }

    /* Setup writer, if none is set (eg. by invoker). */
    if (proc->reader_ == nullptr && params.reader)
    {
        proc->set_reader(std::move(params.reader));
    }

    /* Setup leader, if specified. */
    if (auto ihost = processes_.find(params.leader_id); ihost != processes_.end())
    {
        Proc* leader = ihost->second.get();
        proc->set_leader(leader);
    }

    /* Fork fd:s, if fork is specified. */
    if (auto ifork = processes_.find(params.fork_pid); ifork != processes_.end())
    {
        proc->copy_descriptors_from(*ifork->second.get());
    }

    return proc;
}

void OS::kill_process(int32_t pid)
{
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

bool OS::process_is_running(int32_t pid) const
{
	return processes_.contains(pid);
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

bool OS::serialize(world::Host* to)
{
    to->set_hostname(get_hostname());
    to->set_addr(net_.get_primary_ip().raw);

	world::FileSystem* to_fs = to->mutable_files();
    if (FileSystem* fs = get_filesystem())
        fs->serialize(to_fs);

	return true;
}

bool OS::deserialize(const world::Host& from)
{
    hostname_ = from.hostname();
    // Set Ip

    const world::FileSystem& from_fs = from.files();
    if (FileSystem* fs = get_filesystem())
        fs->deserialize(from_fs);

	return true;
}
