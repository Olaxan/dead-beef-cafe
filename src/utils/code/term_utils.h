#pragma once

#include <string>
#include <format>

#define ESC "\x1b"
#define CSI "\x1b["
#define CSI_CODE(code) "\x1b[" #code "m"
#define CSI_PLACEHOLDER "\x1b[{}m"
#define CSI_RESET CSI_CODE(0)
#define LINE_BEGIN ESC "(0"
#define LINE_END ESC "(B"
#define CSI_RET(code) (code == 0 ? CSI_CODE(32) : CSI_CODE(31))

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

	std::string line(int32_t length);
	std::string msg_line(std::string str, int32_t length);
};