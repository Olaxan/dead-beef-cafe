#include "os_basic.h"
#include "os_input.h"

#include "os.h"
#include "filesystem.h"

#include <ranges>
#include <charconv>

struct LoginPasswdData
{
	int32_t uid;
	int32_t gid;
	std::string_view username{};
	std::string password{};
	std::string gecos{};
	std::string home_dir{};
	std::string shell_path{};
};

std::optional<LoginPasswdData> parse_passwd_row(std::string_view row)
{
	LoginPasswdData data{};
	auto&& range = row | std::views::split(':') | std::ranges::to<std::vector>();

	if (range.size() != 7)
		return std::nullopt;

	data.username = std::string_view(range[0]);
	data.password = std::string_view(range[1]);
	data.gecos = std::string_view(range[4]);
	data.home_dir = std::string_view(range[5]);
	data.shell_path = std::string_view(range[6]);

	std::string_view s_uid(range[2]);
	std::string_view s_gid(range[3]);

    if (auto [ptr, ec] = std::from_chars(s_uid.data(), s_uid.data() + s_uid.size(), data.uid); ec != std::errc())
	{
		data.uid = 0;
	}

	if (auto [ptr, ec] = std::from_chars(s_gid.data(), s_gid.data() + s_gid.size(), data.gid); ec != std::errc())
	{
		data.gid = 0;
	}

	return data;
}

std::optional<LoginPasswdData> get_passwd_data(File* passwd, std::string_view user)
{
	std::string_view f = passwd->get_view();

	for (auto&& rline : f | std::views::split('\n'))
	{
		std::string_view line(rline);
		std::size_t delim = line.find_first_of(':');
		if (line.substr(0, delim).compare(user) == 0)
			return parse_passwd_row(line);
	}

	return std::nullopt;
}

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
		if (std::optional<LoginPasswdData> data = get_passwd_data(ptr, username))
		{
			proc.putln("Login successful.");
			co_return 0;
		}
		else
		{
			proc.warnln("No such user.");
			co_return 1;
		}
	}
	else
	{
		proc.errln("Failed to open passwords file.");
		co_return 2;
	}

	proc.warnln("Invalid credentials.");
	co_return 1;
}
