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

	std::string color(const std::string& str, TermColor fg, TermColor bg = TermColor::Black);
};