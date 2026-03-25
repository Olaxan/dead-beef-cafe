#pragma once

#include "session.h"

#include <unordered_map>
#include <cstdint>

class OS;
class SessionManager
{
public:

	SessionManager() = delete;
	SessionManager(OS* os) : os_(*os) {}

	/* Creates a new user session and returns the sid. */
	int32_t create_session(int32_t uid = 0, int32_t gid = 0);
	
	/* Ends the specified session. Returns whether successful. */
	bool end_session(int32_t sid);

	/* Sets the uid of a session. Returns whether successful. */
	bool set_session_uid(int32_t sid, int32_t new_uid);

	/* Sets the gid of a session. Returns whether successful. */
	bool set_session_gid(int32_t sid, int32_t new_gid);

	/* Add supplementary groups to a session. Returns whether any groups were added. */
	bool add_session_groups(int32_t sid, const std::unordered_set<int32_t>& groups);

	/* Gets the UID for a specific session, or -1 if not found. */
	int32_t get_session_uid(int32_t sid);

	/* Gets the GID for a specific session, or -1 if not found. */
	int32_t get_session_gid(int32_t sid);


protected:

	OS& os_;
	int32_t sid_counter_{0};
	std::unordered_map<int32_t, SessionData> sessions_{};

};