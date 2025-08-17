#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include <unordered_map>

class OS;

struct LoginPasswdData
{
	int32_t uid;
	int32_t gid;
	std::string username{};
	std::string password{};
	std::string gecos{};
	std::string home_dir{};
	std::string shell_path{};
};

struct LoginShadowData
{
	std::string username{};
	std::string password{};
	std::string last_pass_change{};
	std::string min_pass_age{};
	std::string max_pass_age{};
	std::string warning_period{};
	std::string inactivity_period{};
	std::string expiration_date{};
};

struct LoginGroupData
{
	int32_t gid;
	std::string group_name{};
	std::string password{};
	std::string members{};
};

class UsersManager
{
public:

	UsersManager() = delete;
	UsersManager(OS* owning_os) : os_(*owning_os) { };

	void prepare();

	std::optional<std::string_view> get_password_hash(std::string user) const;

	std::optional<LoginPasswdData> authenticate(std::string user, std::string password) const;

protected:

	void get_passwd_data();
	void get_shadow_data();
	void get_group_data();

protected:

	OS& os_;

	int64_t passwd_mod_{0};
	int64_t shadow_mod_{0};
	int64_t groups_mod_{0};

	std::unordered_map<std::string, LoginPasswdData> passwd_;
	std::unordered_map<std::string, LoginShadowData> shadow_;
	std::unordered_map<std::string, LoginGroupData> group_;

};