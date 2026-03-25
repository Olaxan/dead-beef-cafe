#include "session_mgr.h"
#include "session.h"


int32_t SessionManager::create_session(int32_t uid, int32_t gid)
{
    int32_t sid = ++sid_counter_;
	sessions_.emplace(std::make_pair(sid, SessionData{sid, uid, gid}));
    return sid;
}

bool SessionManager::end_session(int32_t sid)
{
    return sessions_.erase(sid);
}

bool SessionManager::set_session_uid(int32_t sid, int32_t new_uid)
{
	if (auto it = sessions_.find(sid); it != sessions_.end())
    {
        it->second.uid = new_uid;
        return true;
    }
    return false;
}

bool SessionManager::set_session_gid(int32_t sid, int32_t new_gid)
{
	if (auto it = sessions_.find(sid); it != sessions_.end())
    {
        it->second.gid = new_gid;
        return true;
    }
    return false;
}

bool SessionManager::add_session_groups(int32_t sid, const std::unordered_set<int32_t>& groups)
{
	if (auto it = sessions_.find(sid); it != sessions_.end())
    {
        it->second.groups.insert(groups.begin(), groups.end());
        return true;
    }
    return false;
}

int32_t SessionManager::get_session_uid(int32_t sid)
{
	if (auto it = sessions_.find(sid); it != sessions_.end())
    {
		return it->second.uid;
    }
    return -1;
}

int32_t SessionManager::get_session_gid(int32_t sid)
{
	if (auto it = sessions_.find(sid); it != sessions_.end())
    {
        return it->second.gid;
    }
    return -1;
}
