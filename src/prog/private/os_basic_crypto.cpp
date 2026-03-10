#include "os_basic.h"
#include "os_input.h"
#include "os_fileio.h"

#include "os.h"
#include "filesystem.h"
#include "sha256.h"
#include "users_mgr.h"

#include "CLI/CLI.hpp"

#include <ranges>
#include <charconv>
#include <unordered_set>

ProcessTask Programs::CmdCrypto(Proc& proc, std::vector<std::string> args)
{
    OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();
	NetManager* net = os.get_network_manager();
	assert(fs);

	CLI::App app{"Utility for hashing and encrypting strings."};
	app.allow_windows_style_options(false);

	struct CryptoArgs
	{
		std::vector<std::string> strings;
	} params{};


	app.add_option("STRINGS", params.strings, "Strings to encrypt");

	try
	{
		std::ranges::reverse(args);
		args.pop_back();
        app.parse(std::move(args));
    }
	catch(const CLI::ParseError& e)
	{
		int res = app.exit(e, proc.s_out, proc.s_err);
        co_return res;
    }

	for (auto& str : params.strings)
	{
		SHA256 sha{};
		sha.update(str);
		std::array<uint8_t, 32> digest = sha.digest();
		std::string digest_str = SHA256::to_string(digest);
	
		proc.putln("{}: <{}>", str, digest_str);
	}

	co_return 0;
}