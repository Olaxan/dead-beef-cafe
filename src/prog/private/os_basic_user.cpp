#include "os_basic.h"
#include "os_input.h"

#include "os.h"
#include "filesystem.h"
#include "sha256.h"

#include <ranges>
#include <charconv>

struct LoginPasswdData
{
	int32_t uid;
	int32_t gid;
	std::string_view username{};
	std::string_view password{};
	std::string_view gecos{};
	std::string_view home_dir{};
	std::string_view shell_path{};
};

struct LoginShadowData
{
	std::string_view username{};
	std::string_view password{};
	std::string_view last_pass_change{};
	std::string_view min_pass_age{};
	std::string_view max_pass_age{};
	std::string_view warning_period{};
	std::string_view inactivity_period{};
	std::string_view expiration_date{};
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

std::optional<LoginShadowData> parse_shadow_row(std::string_view row)
{
	LoginShadowData data{};
	auto&& range = row | std::views::split(':') | std::ranges::to<std::vector>();

	if (range.size() != 9)
		return std::nullopt;

	data.username = std::string_view(range[0]);
	data.password = std::string_view(range[1]);
	data.last_pass_change = std::string_view(range[2]);
	data.min_pass_age = std::string_view(range[3]);
	data.max_pass_age = std::string_view(range[4]);
	data.warning_period = std::string_view(range[5]);
	data.inactivity_period = std::string_view(range[6]);
	data.expiration_date = std::string_view(range[7]);

	return data;
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

	auto get_passwd_data = [&proc, &fs](std::string_view user) -> std::optional<LoginPasswdData>
	{
		if (auto [fid, ptr, err] = fs->open("/etc/passwd"); err == FileSystemError::Success)
		{
			std::string_view f = ptr->get_view();
	
			for (auto&& rline : f | std::views::split('\n'))
			{
				std::string_view line(rline);
				std::size_t delim = line.find_first_of(':');
				if (line.substr(0, delim).compare(user) == 0)
					return parse_passwd_row(line);
			}
		}
		else
		{
			proc.errln("Failed to open passwords file.");
		}

		return std::nullopt;
	};

	auto get_shadow_data = [&proc, &fs](const std::string_view user) -> std::optional<LoginShadowData>
	{
		if (auto [fid, ptr, err] = fs->open("/etc/shadow"); err == FileSystemError::Success)
		{
			std::string_view f = ptr->get_view();
	
			for (auto&& rline : f | std::views::split('\n'))
			{
				std::string_view line(rline);
				std::size_t delim = line.find_first_of(':');
				if (line.substr(0, delim).compare(user) == 0)
					return parse_shadow_row(line);
			}
		}
		else
		{
			proc.errln("Failed to open shadow file.");
		}

		return std::nullopt;
	};

	auto get_password_hash = [](const LoginPasswdData& passwd, const LoginShadowData& shadow) -> std::string_view
	{
		return (passwd.password == "x") ? shadow.password : passwd.password;
	};

	CmdInput::CmdReaderParams usr_params{};
	CmdInput::CmdReaderParams pwd_params{.password = true};

	proc.put("Username: ");
	std::string username = co_await CmdInput::read_cmd_utf8(proc, usr_params);

	proc.put("Password: ");
	std::string password = co_await CmdInput::read_cmd_utf8(proc, pwd_params);

	std::optional<LoginPasswdData> passwd = get_passwd_data(username);
	std::optional<LoginShadowData> shadow = get_shadow_data(username);

	if (passwd && shadow)
	{
		std::string_view pub = get_password_hash(*passwd, *shadow);

		SHA256 sha{};
		sha.update(password);
		std::array<uint8_t, 32> digest = sha.digest();

		std::string digest_str = SHA256::to_string(digest);

		if (pub.compare(digest_str) == 0)
		{
			/* Auth successful. */

			proc.set_sid();
			proc.set_uid(passwd->uid);
			proc.set_gid(passwd->gid);

			proc.set_var("HOME", passwd->home_dir);
			proc.set_var("SHELL", passwd->shell_path);
			proc.set_var("USER", passwd->username);

			proc.putln("Welcome, {}.", username);
			co_return 0;
		}
		else
		{
			/* Password incorrect. */
			proc.warnln("Invalid credentials (SHA={}).", digest_str);
			co_return 1;
		}
	}
	else
	{
		/* Not found in user registry. */
		proc.warnln("No such user.");
		co_return 2;
	}
}
