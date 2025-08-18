#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

class OS;

struct GecosData
{
	std::string full_name{};
	std::string room_no{};
	std::string work_phone{};
	std::string home_phone{};
	std::string other{};
};

struct LoginPasswdData
{
	int32_t uid;
	int32_t gid;
	std::string username{};
	std::string password{};
	std::string home_path{};
	std::string shell_path{};
	GecosData gecos{};
};

struct LoginShadowData
{
	std::string username{};
	std::string password{};
	uint64_t last_pass_change{};
	uint64_t expiration_date{};
	int32_t min_pass_age{};
	int32_t max_pass_age{};
	int32_t warning_period{};
	int32_t inactivity_period{};
};

struct LoginGroupData
{
	int32_t gid;
	std::string group_name{};
	std::string password{};
	std::vector<std::string> members{};
};

struct CreateUserParams
{
	bool create_home{true};
	bool create_usergroup{true};
	int32_t uid{-1};
	int32_t gid{-1};
	int32_t warning_period{};
	int32_t inactivity_period{};
	uint64_t expiration_date{};
	std::string shell_path{};
	std::string home_path{};
	GecosData gecos{};
	std::vector<std::string> groups{};
};

struct CreateGroupParams
{
	int32_t gid{0};
	std::vector<std::string> users{};
};

class UsersManager
{
public:

	UsersManager() = delete;
	UsersManager(OS* owning_os) : os_(*owning_os) { };

	/* Prepare for a database query, i.e. make sure the in-memory data is fresh,
	or retrieve it anew otherwise. */
	void prepare();

	/* Write the in-memory data to database files. */
	void commit();

	/* Get the password hash for the provided user (from passwd/shadow cache). */
	std::optional<std::string_view> get_password_hash(int32_t uid) const;

	/* Returns the username of a UID, if one one exists. */
	std::optional<std::string_view> get_username(int32_t uid) const;

	/* Returns the group name of a GID, if one exists. */
	std::optional<std::string_view> get_group_name(int32_t gid) const;

	/* Retrieve user data for a valid username/password pair. */
	std::optional<LoginPasswdData> authenticate(std::string user, std::string password) const;

	/* Adds a new user to memory. If auto-commit is set, the passwd/shadow/group db's will also be updated.*/
	bool add_user(std::string username, std::string password, CreateUserParams&& params, bool auto_commit = true);

	/* Adds a new group to memory. If auto-commit is set, the groups/gshadow db's will also be updated.*/
	bool add_group(std::string group_name, CreateGroupParams&& params, bool auto_commit = true);

	/* Add the specified user to a group, returning whether successful. */
	bool add_user_to_group(std::string user, std::string group);

	/* Check if the user is a member of a specified group. */
	bool check_belongs(int32_t uid, int32_t gid);

protected:

	void get_passwd_data();
	void get_shadow_data();
	void get_group_data();

	void write_passwd_data();
	void write_shadow_data();
	void write_group_data();

protected:

	OS& os_;

	int32_t uid_counter_{1000};
	int32_t gid_counter_{1000};

	int64_t passwd_mod_{0};
	int64_t shadow_mod_{0};
	int64_t groups_mod_{0};

	std::unordered_map<std::string, int32_t> name_to_uid_;
	std::unordered_map<std::string, int32_t> name_to_gid_;

	std::unordered_map<int32_t, LoginPasswdData> passwd_;
	std::unordered_map<int32_t, LoginShadowData> shadow_;
	std::unordered_map<int32_t, LoginGroupData> groups_;

};