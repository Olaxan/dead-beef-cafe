#pragma once

#include <string>
#include <format>

enum class TermColor
{
	Black,
	Red,
	Green,
	Yellow,
	Blue,
	Magenta,
	Cyan,
	White,
	BrightBlack,
	BrightRed,
	BrightGreen,
	BrightYellow,
	BrightBlue,
	BrightMagenta,
	BrightCyan,
	BrightWhite,
};

namespace TermUtils
{

	int32_t get_ansi_fg_color(TermColor color);

	int32_t get_ansi_bg_color(TermColor color);

	template<typename ...Args>
	std::string color(std::format_string<Args...> fmt, Args&& ...args, TermColor fg, TermColor bg = TermColor::Black)
	{
		return std::format("\x1B[{};{}m{}\x1B[0m", 
			TermUtils::get_ansi_fg_color(fg), TermUtils::get_ansi_bg_color(bg), std::format(fmt, std::forward<Args>(args)...));
	}
};