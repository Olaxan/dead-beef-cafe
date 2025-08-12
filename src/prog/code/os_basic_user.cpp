#include "os_basic.h"
#include "os_input.h"

#include "os.h"
#include "filesystem.h"

ProcessTask Programs::CmdLogin(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No filesystem!");
		co_return 2;
	}

	CmdInput::CmdReaderParams usr_params{};
	CmdInput::CmdReaderParams pwd_params{.password = true};

	proc.put("Username: ");
	std::string username = co_await CmdInput::read_cmd_utf8(proc, usr_params);

	proc.put("Password: ");
	std::string password = co_await CmdInput::read_cmd_utf8(proc, pwd_params);

	if (auto [fid, ptr, err] = fs->open("/etc/passwd", FileAccessFlags::Read); err == FileSystemError::Success)
	{

	}

	proc.warnln("Invalid credentials.");
	co_return 1;
}
