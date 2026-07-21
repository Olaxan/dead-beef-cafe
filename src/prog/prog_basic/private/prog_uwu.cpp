#include "prog_basic.h"
#include "term_utils.h"

#include "CLI/CLI.hpp"

#include <string>

std::string uwuify(std::string_view in)
{
	using namespace std::string_view_literals;

	return in
	| std::views::split("u"sv) 
	| std::views::join_with("owo"sv)
	| std::ranges::to<std::string>();
}

std::string owoify(std::string_view in)
{
	using namespace std::string_view_literals;

	return in
	| std::views::split("o"sv) 
	| std::views::join_with("owo"sv)
	| std::ranges::to<std::string>();
}

ProcessTask Programs::CmdUwu(Proc& proc, std::vector<std::string> args)
{
    CLI::App app{"A UwUsefUwUl tOwOOwOl tOwO replace UwU with UwUwUwU and OwO with OwOwOwO."};
	app.allow_windows_style_options(false);

	struct SpeechArgs
	{
		std::string line{"*Notices your bulge*"};
	} params{};

	app.add_option("LINE", params.line, "The line to replace");

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

	if (proc.is_tty())
	{
		std::string safe = TermUtils::strip_ansi(params.line);
		std::string uwu = uwuify(safe);
		std::string owo = owoify(uwu);
		proc.write(owo);
		proc.putln("");
	}
	else
	{
		while (true)
		{
			auto res = co_await proc.read();

			if (not res)
			{
				co_return 0;
			}

			std::string safe = TermUtils::strip_ansi(*res);
			std::string uwu = uwuify(safe);
			std::string owo = owoify(uwu);
			proc.write(owo);
		}
	}

    co_return 0;
}