#pragma once

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/ustring.h>
#include <unicode/ustream.h>
#include <unicode/brkiter.h>

#include <string>
#include <format>

#define ESC "\x1b"
#define CSI "\x1b["
#define DEL "\x7f"
#define CTRL_KEY(k) ((k) & 0x1f)
#define OSC_BEGIN(code) "\x1b]" #code ";"
#define OSC_END "\x1b\x5c"
#define CSI_CODE(code) "\x1b[" #code "m"
#define CSI_PLACEHOLDER "\x1b[{}m"
#define CSI_RESET CSI_CODE(0)
#define LINE_BEGIN ESC "(0"
#define LINE_END ESC "(B"
#define BEGIN_ALT_SCREEN_BUFFER CSI "?1049h"
#define END_ALT_SCREEN_BUFFER CSI "?1049l"
#define CLEAR_SCREEN CSI "2J"
#define CLEAR_CURSOR CSI "H"
#define MOVE_CURSOR(x, y) CSI #y ";" #x "H"
#define MOVE_CURSOR_FORMAT(x, y) std::format(CSI "{};{}H", y, x)
#define MOVE_CURSOR_PLACEHOLDER CSI "{};{}H"
#define SHOW_CURSOR CSI "?25h"
#define HIDE_CURSOR CSI "?25l"
#define ENABLE_CURSOR_BLINK CSI "?12h"
#define DISABLE_CURSOR_BLINK CSI "?12l"
#define CURSOR_UP CSI "A"
#define CURSOR_DOWN CSI "B"
#define CURSOR_RIGHT CSI "C"
#define CURSOR_LEFT CSI "D"
#define DELETE_CHAR CSI "P"
#define ERASE_CHAR CSI "X"
#define REVERSE_VIDEO CSI_CODE(7)



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

//class icu::UnicodeString;

namespace TermUtils
{

	int32_t get_ansi_fg_color(TermColor color);

	int32_t get_ansi_bg_color(TermColor color);

	std::string color(const std::string& str, TermColor fg, TermColor bg = TermColor::Black);

	std::string pixel(int32_t x, int32_t y, std::string c);
	std::string line(int32_t length);
	std::string msg_line(std::string str, int32_t length);
	std::string status_line(int32_t line, int32_t width, std::string left, std::string right);
	std::string status_line(int32_t line, int32_t width, const icu::UnicodeString& left, std::string right);
	std::string status_line(int32_t line, int32_t width, const icu::UnicodeString& left, const icu::UnicodeString& right);
	std::string line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, std::string out_char);
	std::string frame(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
	std::string frame(int32_t x1, int32_t y1, int32_t x2, int32_t y2, std::string msg);
};