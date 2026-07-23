#include "prog_basic.h"
#include "term_utils.h"

#include "CLI/CLI.hpp"

#include <string>

struct Replacement
{
    std::string_view from;
    std::string_view to;
};

std::string replace_all(std::string_view input, std::span<const Replacement> replacements)
{
    // Longest matches first so "foobar" beats "foo".
    std::vector<Replacement> rules(replacements.begin(), replacements.end());

    std::ranges::sort(rules,
        [](auto const& a, auto const& b)
        {
            return a.from.size() > b.from.size();
        });

    std::string result;
    result.reserve(input.size());

    std::size_t pos = 0;

    while (pos < input.size())
    {
        bool matched = false;

        for (auto const& rule : rules)
        {
            if (input.substr(pos).starts_with(rule.from))
            {
                result += rule.to;
                pos += rule.from.size();
                matched = true;
                break;
            }
        }

        if (!matched)
        {
            result += input[pos++];
        }
    }

    return result;
}

std::string owoify(std::string_view in)
{
	using namespace std::string_view_literals;

	return replace_all(in, {{
		{ "r"sv, "ww"sv },
		{ "l"sv, "w"sv },
		{ "R"sv, "W"sv },
		{ "L"sv, "W"sv },
		{ "ove"sv, "uv"sv },
		{ "N"sv, "Ny"sv },
		{ "?"sv, "? OwO"sv },
		{ "!"sv, "!! UwU"sv },
		{"."sv, ". >w<"sv },
		{"..."sv, "... >w<"sv },
		{"A"sv, "A-a"sv },
		{"I"sv, "I-i"sv },
		{"H"sv, "H-h"sv },
	}});
}

ProcessTask Programs::CmdUwu(Proc& proc, std::vector<std::string> args)
{
    CLI::App app{"A usefuw toow foww making ur text uwu."};
	app.allow_windows_style_options(false);

	struct SpeechArgs
	{
		std::string line{"*Notices your bulge*"};
		bool no_strip{false};
	} params{};

	app.add_option("LINE", params.line, "The line to replace");
	app.add_flag("--no-strip", params.no_strip, "Do not strip ANSI escape codes");

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
		std::string safe = params.no_strip ? params.line : TermUtils::strip_ansi(params.line);
		std::string owo = owoify(safe);
		proc.write(owo);
		proc.put("\n");
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

			std::string safe = params.no_strip ? *res : TermUtils::strip_ansi(*res);
			std::string owo = owoify(safe);
			proc.write(owo);
		}
	}

    co_return 0;
}

// Allow calling with a braced-init-list of Replacement
std::string replace_all(std::string_view input, std::initializer_list<Replacement> replacements)
{
	return replace_all(input, std::span<const Replacement>(replacements.begin(), replacements.size()));
}