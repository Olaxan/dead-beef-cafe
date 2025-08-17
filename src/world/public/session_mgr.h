#pragma once

class OS;
class SessionManager
{
public:

	SessionManager() = delete;
	SessionManager(OS* os) : os_(*os) {}

protected:

	OS& os_;
};