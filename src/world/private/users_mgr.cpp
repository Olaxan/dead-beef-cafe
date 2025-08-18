#include "users_mgr.h"

#include "os.h"
#include "filesystem.h"
#include "sha256.h"

#include <ranges>
#include <string_view>
#include <charconv>
#include <unordered_set>
#include <optional>
#include <format>


std::optional<LoginPasswdData> parse_passwd_row(std::string_view row)
{
	LoginPasswdData data{};
	auto&& range = row | std::views::split(':') | std::ranges::to<std::vector>();

	if (range.size() != 7)
		return std::nullopt;

	data.username = std::string_view(range[0]);
	data.password = std::string_view(range[1]);
	data.home_path = std::string_view(range[5]);
	data.shell_path = std::string_view(range[6]);

	data.gecos = std::invoke([](std::string_view input) -> GecosData
	{
		auto&& range = input | std::views::split(',') | std::ranges::to<std::vector>();
		
		if (range.size() != 5)
			return {};

		GecosData out{};
		out.full_name = std::string_view(range[0]);
		out.room_no = std::string_view(range[1]);
		out.work_phone = std::string_view(range[2]);
		out.home_phone = std::string_view(range[3]);
		out.other = std::string_view(range[4]);

		return out;
	}, std::string_view(range[4]));

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

	std::string_view sv_change(range[2]);
	if (auto [ptr, ec] = std::from_chars(sv_change.data(), sv_change.data() + sv_change.size(), data.last_pass_change); ec != std::errc())
	{
		data.last_pass_change = 0;
	}

	std::string_view sv_minpass(range[3]);
	if (auto [ptr, ec] = std::from_chars(sv_minpass.data(), sv_minpass.data() + sv_minpass.size(), data.min_pass_age); ec != std::errc())
	{
		data.min_pass_age = 0;
	}

	std::string_view sv_maxpass(range[4]);
	if (auto [ptr, ec] = std::from_chars(sv_maxpass.data(), sv_maxpass.data() + sv_maxpass.size(), data.max_pass_age); ec != std::errc())
	{
		data.max_pass_age = 0;
	}

	std::string_view sv_warn(range[5]);
	if (auto [ptr, ec] = std::from_chars(sv_warn.data(), sv_warn.data() + sv_warn.size(), data.warning_period); ec != std::errc())
	{
		data.warning_period = 0;
	}

	std::string_view sv_inac(range[6]);
	if (auto [ptr, ec] = std::from_chars(sv_inac.data(), sv_inac.data() + sv_inac.size(), data.inactivity_period); ec != std::errc())
	{
		data.inactivity_period = 0;
	}

	std::string_view sv_exp(range[7]);
	if (auto [ptr, ec] = std::from_chars(sv_exp.data(), sv_exp.data() + sv_exp.size(), data.expiration_date); ec != std::errc())
	{
		data.expiration_date = 0;
	}

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

	data.members = std::string_view(range[3])
	| std::views::split(',')
	| std::ranges::to<std::vector<std::string>>();
	
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
	if (uint64_t fid = fs->get_fid("/etc/group"); fid != 0)
	{
		if (uint64_t mod = fs->get_last_modified(fid); mod != groups_mod_)
		{
			get_group_data();
			groups_mod_ = mod;
		}
	}
}

void UsersManager::commit()
{
	write_passwd_data();
	write_shadow_data();
	write_group_data();
}

std::optional<std::string_view> UsersManager::get_password_hash(int32_t uid) const
{
	if (auto&& p = passwd_.find(uid); p != passwd_.end())
	{
		if (p->second.password != "x")
			return std::string_view(p->second.password);

		if (auto&& s = shadow_.find(uid); s != shadow_.end())
			return std::string_view(s->second.password);
	}

	return std::nullopt;
}

std::optional<std::string_view> UsersManager::get_username(int32_t uid) const
{
	if (auto&& it = passwd_.find(uid); it != passwd_.end())
	{
		return std::string_view(it->second.username);
	}

	return std::nullopt;
}

std::optional<std::string_view> UsersManager::get_group_name(int32_t gid) const
{
	if (auto&& it = groups_.find(gid); it != groups_.end())
	{
		return std::string_view(it->second.group_name);
	}

	return std::nullopt;
}

std::optional<LoginPasswdData> UsersManager::authenticate(std::string user, std::string password) const
{
	if (auto it = name_to_uid_.find(std::move(user)); it != name_to_uid_.end())
	{
		int32_t uid = it->second;
		if (std::optional<std::string_view> opt_hash = get_password_hash(uid); opt_hash.has_value())
		{
			SHA256 sha{};
			sha.update(std::move(password));
			std::array<uint8_t, 32> digest = sha.digest();
			std::string digest_str = SHA256::to_string(digest);
		
			if (opt_hash->compare(digest_str) == 0)
				return passwd_.at(uid);
		}
	}	

	return std::nullopt;
}

bool UsersManager::add_user(std::string username, std::string password, CreateUserParams&& params, bool auto_commit)
{
	FileSystem* fs = os_.get_filesystem();
	assert(fs);

	if (name_to_uid_.contains(username))
		return false;

	int32_t new_uid = (params.uid == -1) ? ++uid_counter_ : params.uid;
	int32_t new_gid = (params.gid == -1) ? ++gid_counter_ : params.gid;

	name_to_uid_[username] = new_uid;

	passwd_[new_uid] = LoginPasswdData{
		.uid = new_uid,
		.gid = new_gid,
		.username = username,
		.password = "x",
		.home_path = std::move(params.home_path),
		.shell_path = std::move(params.shell_path),
		.gecos = std::move(params.gecos)
	};

	std::string crypt = std::invoke([](std::string&& password)
	{
		SHA256 sha{};
		sha.update(std::move(password));
		std::array<uint8_t, 32> digest = sha.digest();
		std::string digest_str = SHA256::to_string(digest);
		return digest_str;
	}, std::move(password)); // Password invalid state after this.

	shadow_[new_uid] = LoginShadowData{
		.username = username,
		.password = std::move(crypt),
		.last_pass_change = 0,
		.expiration_date = params.expiration_date,
		.min_pass_age = 0,
		.max_pass_age = 0,
		.warning_period = params.warning_period,
		.inactivity_period = params.inactivity_period,
	};

	for (auto&& group : params.groups)
	{
		bool success = add_user_to_group(username, group);
		std::println("Added '{}' to '{}': {}", username, group, success);
	}

	if (params.create_home)
	{
		FilePath home = [&params](std::string_view name)
		{
			if (params.home_path.empty())
				return std::format("/home/{}", name);

			return params.home_path;
		}(username);

		fs->create_directory(home, 
		{
			.recurse = true,
			.meta = {
				.owner_uid = new_uid,
				.owner_gid = new_gid,
				.perm_owner = FilePermissionTriad::All,
				.perm_group = FilePermissionTriad::None,
				.perm_users = FilePermissionTriad::None,
				.extra = ExtraFileFlags::Directory
			}
		});
	}

	if (params.create_usergroup)
	{
		add_group(username, 
		{
			.gid = new_gid,
			.users = { username }
		}, false);
	}

	if (auto_commit)
	{
		write_passwd_data();
		write_shadow_data();
		write_group_data();
	}

	return true;
}

bool UsersManager::add_group(std::string group_name, CreateGroupParams&& params, bool auto_commit)
{
	FileSystem* fs = os_.get_filesystem();
	assert(fs);

	if (name_to_gid_.contains(group_name))
		return false;

	int32_t new_gid = (params.gid == -1) ? ++gid_counter_ : params.gid;
	
	name_to_gid_[group_name] = new_gid;

	groups_[new_gid] = LoginGroupData{
		.gid = params.gid,
		.group_name = group_name,
		.password = "x",
		.members = std::move(params.users)
	};

	if (auto_commit)
	{
		write_group_data();
	}

	return true;
}

bool UsersManager::add_user_to_group(std::string user, std::string group)
{
	if (auto it_gid = name_to_gid_.find(std::move(group)); it_gid != name_to_gid_.end())
	{
		if (auto it_grp = groups_.find(it_gid->second); it_grp != groups_.end())
		{
			it_grp->second.members.push_back(std::move(user));
			return true;
		}
	}

	return false;
}

bool UsersManager::check_belongs(int32_t uid, int32_t gid)
{
	if (auto group_it = groups_.find(gid); group_it != groups_.end())
	{
		/* Not ideal, we have to loop through and do a bunch of hash lookups, 
		but will have to suffice until we can cache this stuff somewhere. */
		for (auto&& member : group_it->second.members)
		{
			if (auto uid_it = name_to_uid_.find(member); uid_it != name_to_uid_.end() && uid_it->second == uid)
			{
				return true;
			}
		}
	}

	return false;
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
				name_to_uid_[opt_data->username] = opt_data->uid;
				passwd_[opt_data->uid] = std::move(*opt_data);
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
				if (auto it = name_to_uid_.find(opt_data->username); it != name_to_uid_.end())
				{
					shadow_[it->second] = std::move(*opt_data);
				}
			}
		}
	}
}

void UsersManager::get_group_data()
{
	FileSystem* fs = os_.get_filesystem();
	assert(fs);

	groups_.clear();

	if (auto [fid, ptr, err] = fs->open("/etc/group", FileAccessFlags::Read); err == FileSystemError::Success)
	{
		std::string_view f = ptr->get_view();

		for (auto&& rline : f | std::views::split('\n'))
		{
			std::string_view line(rline);
			if (std::optional<LoginGroupData> opt_data = parse_group_row(line); opt_data.has_value())
			{
				name_to_gid_[opt_data->group_name] = opt_data->gid;
				groups_[opt_data->gid] = std::move(*opt_data);
			}
		}
	}
}

void UsersManager::write_passwd_data()
{
	FileSystem* fs = os_.get_filesystem();
	assert(fs);

	if (auto [fid, ptr, err] = fs->open("/etc/passwd", FileAccessFlags::Write); err == FileSystemError::Success)
	{
		ptr->write("");

		for (auto&& [username, entry] : passwd_)
		{
			const GecosData& in_gecos = entry.gecos;
			std::string gecos_str = std::format("{},{},{},{},{}",
			in_gecos.full_name,
			in_gecos.room_no,
			in_gecos.work_phone,
			in_gecos.home_phone,
			in_gecos.other);

			std::string passwd = std::format("{}:{}:{}:{}:{}:{}:{}\n",
			entry.username,
			"x",
			entry.uid,
			entry.gid,
			std::move(gecos_str),
			entry.home_path,
			entry.shell_path);
			
			ptr->append(std::move(passwd));
		}	
	}
}

void UsersManager::write_shadow_data()
{
	FileSystem* fs = os_.get_filesystem();
	assert(fs);

	if (auto [fid, ptr, err] = fs->open("/etc/shadow", FileAccessFlags::Write); err == FileSystemError::Success)
	{
		ptr->write("");

		for (auto&& [username, entry] : shadow_)
		{
			std::string shadow = std::format("{}:{}:{}:{}:{}:{}:{}:{}:\n",
			entry.username,
			entry.password,
			entry.last_pass_change,
			entry.min_pass_age,
			entry.max_pass_age,
			entry.warning_period,
			entry.inactivity_period,
			entry.expiration_date);

			ptr->append(std::move(shadow));
		}	
	}
}

void UsersManager::write_group_data()
{
	FileSystem* fs = os_.get_filesystem();
	assert(fs);

	if (auto [fid, ptr, err] = fs->open("/etc/group", FileAccessFlags::Write); err == FileSystemError::Success)
	{
		ptr->write("");

		for (auto&& [group_name, entry] : groups_)
		{
			std::string group = std::format("{}:{}:{}:{}\n",
			entry.group_name,
			entry.password,
			entry.gid,
			entry.members | std::views::join_with(',') | std::ranges::to<std::string>());

			ptr->append(std::move(group));
		}
	}
}
