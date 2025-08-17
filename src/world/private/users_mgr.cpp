#include "users_mgr.h"

#include "os.h"
#include "filesystem.h"
#include "sha256.h"

#include <ranges>
#include <string_view>
#include <charconv>
#include <unordered_set>
#include <optional>


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
		data.uid = -1;
	}

	if (auto [ptr, ec] = std::from_chars(s_gid.data(), s_gid.data() + s_gid.size(), data.gid); ec != std::errc())
	{
		data.gid = -1;
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

std::optional<LoginGroupData> parse_group_row(std::string_view row)
{
	LoginGroupData data{};
	auto&& range = row | std::views::split(':') | std::ranges::to<std::vector>();

	if (range.size() != 4)
		return std::nullopt;

	data.group_name = std::string_view(range[0]);
	data.password = std::string_view(range[1]);
	data.members = std::string_view(range[3]);
	
	std::string_view s_gid(range[2]);

    if (auto [ptr, ec] = std::from_chars(s_gid.data(), s_gid.data() + s_gid.size(), data.gid); ec != std::errc())
	{
		data.gid = -1;
	}

	return data;
}

void UsersManager::prepare()
{
	FileSystem* fs = os_.get_filesystem();
	assert(fs);

	/* Refresh passwords database, if it has been modified since last accessed. */
	if (uint64_t fid = fs->get_fid("/etc/passwd"); fid != 0)
	{
		if (uint64_t mod = fs->get_last_modified(fid); mod != passwd_mod_)
		{
			get_passwd_data();
			passwd_mod_ = mod;
		}
	}

	/* Refresh shadow database. */
	if (uint64_t fid = fs->get_fid("/etc/shadow"); fid != 0)
	{
		if (uint64_t mod = fs->get_last_modified(fid); mod != shadow_mod_)
		{
			get_shadow_data();
			shadow_mod_ = mod;
		}
	}

	/* Refresh groups database. */
	if (uint64_t fid = fs->get_fid("/etc/groups"); fid != 0)
	{
		if (uint64_t mod = fs->get_last_modified(fid); mod != groups_mod_)
		{
			get_group_data();
			groups_mod_ = mod;
		}
	}
}

std::optional<std::string_view> UsersManager::get_password_hash(std::string user) const
{
	if (auto&& p = passwd_.find(user); p != passwd_.end())
	{
		if (p->second.password != "x")
			return std::string_view(p->second.password);

		if (auto&& s = shadow_.find(user); s != shadow_.end())
			return std::string_view(s->second.password);
	}

	return std::nullopt;
}

std::optional<LoginPasswdData> UsersManager::authenticate(std::string user, std::string password) const
{
	if (std::optional<std::string_view> opt_hash = get_password_hash(user); opt_hash.has_value())
	{
		SHA256 sha{};
		sha.update(std::move(password));
		std::array<uint8_t, 32> digest = sha.digest();
		std::string digest_str = SHA256::to_string(digest);
	
		 if (opt_hash->compare(digest_str) == 0)
		 	return passwd_.at(user);
	}

	return std::nullopt;
}

void UsersManager::get_passwd_data()
{
	FileSystem* fs = os_.get_filesystem();
	assert(fs);

	passwd_.clear();

	if (auto [fid, ptr, err] = fs->open("/etc/passwd", FileAccessFlags::Read); err == FileSystemError::Success)
	{
		std::string_view f = ptr->get_view();
		
		for (auto&& rline : f | std::views::split('\n'))
		{
			std::string_view line(rline);
			if (std::optional<LoginPasswdData> opt_data = parse_passwd_row(line); opt_data.has_value())
			{
				passwd_[opt_data->username] = std::move(*opt_data);
			}
		}
	}
}

void UsersManager::get_shadow_data()
{
	FileSystem* fs = os_.get_filesystem();
	assert(fs);

	shadow_.clear();

	if (auto [fid, ptr, err] = fs->open("/etc/shadow", FileAccessFlags::Read); err == FileSystemError::Success)
	{
		std::string_view f = ptr->get_view();

		for (auto&& rline : f | std::views::split('\n'))
		{
			std::string_view line(rline);
			if (std::optional<LoginShadowData> opt_data = parse_shadow_row(line); opt_data.has_value())
			{
				shadow_[opt_data->username] = std::move(*opt_data);
			}
		}
	}
}

void UsersManager::get_group_data()
{
	FileSystem* fs = os_.get_filesystem();
	assert(fs);

	group_.clear();

	if (auto [fid, ptr, err] = fs->open("/etc/groups", FileAccessFlags::Read); err == FileSystemError::Success)
	{
		std::string_view f = ptr->get_view();

		for (auto&& rline : f | std::views::split('\n'))
		{
			std::string_view line(rline);
			if (std::optional<LoginGroupData> opt_data = parse_group_row(line); opt_data.has_value())
			{
				group_[opt_data->group_name] = std::move(*opt_data);
			}
		}
	}
}
