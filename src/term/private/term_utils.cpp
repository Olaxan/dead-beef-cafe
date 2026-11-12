#include "term_utils.h"

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/ustring.h>
#include <unicode/ustream.h>
#include <unicode/brkiter.h>

#include <sstream>
#include <regex>
#include <algorithm>

namespace
{
	int32_t ansi_fg_code(TermColor color)
	{
		switch (color)
		{
			case TermColor::Black: return 30;
			case TermColor::Red: return 31;
			case TermColor::Green: return 32;
			case TermColor::Yellow: return 33;
			case TermColor::Blue: return 34;
			case TermColor::Magenta: return 35;
			case TermColor::Cyan: return 36;
			case TermColor::White: return 37;
			case TermColor::BrightBlack: return 90;
			case TermColor::BrightRed: return 91;
			case TermColor::BrightGreen: return 92;
			case TermColor::BrightYellow: return 93;
			case TermColor::BrightBlue: return 94;
			case TermColor::BrightMagenta: return 95;
			case TermColor::BrightCyan: return 96;
			case TermColor::BrightWhite: return 97;
			default: return 37;
		}
	}

	int32_t ansi_bg_code(TermColor color)
	{
		switch (color)
		{
			case TermColor::Black: return 40;
			case TermColor::Red: return 41;
			case TermColor::Green: return 42;
			case TermColor::Yellow: return 43;
			case TermColor::Blue: return 44;
			case TermColor::Magenta: return 45;
			case TermColor::Cyan: return 46;
			case TermColor::White: return 47;
			case TermColor::BrightBlack: return 100;
			case TermColor::BrightRed: return 101;
			case TermColor::BrightGreen: return 102;
			case TermColor::BrightYellow: return 103;
			case TermColor::BrightBlue: return 104;
			case TermColor::BrightMagenta: return 105;
			case TermColor::BrightCyan: return 106;
			case TermColor::BrightWhite: return 107;
			default: return 40;
		}
	}

	std::size_t visible_width(std::string_view value)
	{
		const std::string plain = TermUtils::strip_ansi(std::string(value));
		return static_cast<std::size_t>(icu::UnicodeString::fromUTF8(plain).countChar32());
	}

	std::size_t leading_padding(std::size_t width, std::size_t content_width, TermUtils::TermJustification justification)
	{
		const std::size_t padding = width > content_width ? width - content_width : 0;
		switch (justification)
		{
			case TermUtils::TermJustification::Center: return padding / 2;
			case TermUtils::TermJustification::Right: return padding;
			case TermUtils::TermJustification::Left: return 0;
		}
		return 0;
	}

	std::string padding(int32_t width, std::size_t left, std::size_t right)
	{
		const std::size_t content_width = left + right + 2;
		const std::size_t line_width = width > 0 ? static_cast<std::size_t>(width) : 0;
		return std::string(line_width > content_width ? line_width - content_width : 0, ' ');
	}

	TermUtils::Effect make_effect(std::string begin, std::string end)
	{
		return [begin = std::move(begin), end = std::move(end)](std::string_view value)
		{
			std::string result;
			result.reserve(begin.size() + value.size() + end.size());
			result.append(begin);
			result.append(value);
			result.append(end);
			return result;
		};
	}
}

TermUtils::Text TermUtils::text(std::string value)
{
	return Text{std::move(value)};
}

TermUtils::Text TermUtils::operator|(Text value, const Effect& effect)
{
	return Text{effect(value.str())};
}

TermUtils::Effect TermUtils::operator|(Effect first, Effect second)
{
	return [first = std::move(first), second = std::move(second)](std::string_view value)
	{
		return first(second(value));
	};
}

TermUtils::Effect TermUtils::foreground(TermColor color)
{
	return make_effect(std::format(CSI "{}m", ansi_fg_code(color)), CSI "39m");
}

TermUtils::Effect TermUtils::background(TermColor color)
{
	return make_effect(std::format(CSI "{}m", ansi_bg_code(color)), CSI "49m");
}

TermUtils::Effect TermUtils::color(TermColor fg, TermColor bg)
{
	return background(bg) | foreground(fg);
}

TermUtils::Effect TermUtils::bold()
{
	return make_effect(CSI "1m", CSI "22m");
}

TermUtils::Effect TermUtils::underline()
{
	return make_effect(CSI "4m", CSI "24m");
}

TermUtils::Effect TermUtils::reverse_video()
{
	return make_effect(CSI "7m", CSI "27m");
}

TermUtils::Effect TermUtils::line_graphics()
{
	return make_effect(LINE_BEGIN, LINE_END);
}

std::string TermUtils::strip_ansi(const std::string& in)
{
	static const std::regex ansi_regex(R"(\x1B\[[0-?]*[ -/]*[@-~])");
	return std::regex_replace(in, ansi_regex, "");
}

TermUtils::Text TermUtils::horizontal_line(int32_t length, char glyph)
{
	return text(std::string(std::max(0, length), glyph));
}

TermUtils::Text TermUtils::bar_text(int32_t length)
{
	return text(std::string(std::max(0, length), ' ')) | reverse_video();
}

TermUtils::Text TermUtils::bar_text(std::string label, int32_t length, TermJustification justification)
{
	const std::size_t label_width = visible_width(label);
	const std::size_t bar_width = std::max(
		static_cast<std::size_t>(std::max(0, length)),
		label_width);
	const std::size_t left = leading_padding(bar_width, label_width, justification);
	std::string value(left, ' ');
	value.append(label);
	value.append(bar_width - label_width - left, ' ');
	return text(std::move(value)) | reverse_video();
}

TermUtils::Text TermUtils::heading(std::string title, int32_t width)
{
	width = std::max(0, width);
	const std::size_t title_size = title.size();
	const std::size_t line_size = static_cast<std::size_t>(width);
	const std::size_t padding = line_size > title_size ? (line_size - title_size) / 2 : 0;
	std::string value(padding, ' ');
	value.append(title);
	value.push_back('\n');
	value.append(line_size, '-');
	return text(std::move(value)) | bold();
}

TermUtils::Text TermUtils::pixel(int32_t x, int32_t y, std::string c)
{
	return text(std::format(MOVE_CURSOR_PLACEHOLDER "{}", y, x, c));
}

TermUtils::Text TermUtils::line(int32_t length)
{
	return text(std::string(std::max(0, length), 'q')) | line_graphics();
}

TermUtils::Text TermUtils::msg_line(std::string str, int32_t length)
{
	return msg_line(std::move(str), length, TermJustification::Left);
}

TermUtils::Text TermUtils::msg_line(std::string str, int32_t width, TermJustification justification)
{
	const std::size_t label_width = visible_width(str);

	if (label_width == 0)
		return line(width);

	const std::size_t line_width = std::max(
		static_cast<std::size_t>(std::max(0, width)),
		label_width + 2);
	const std::size_t fill_width = line_width - label_width - 2;
	const std::size_t left_fill = leading_padding(fill_width, 0, justification);
	const std::size_t right_fill = fill_width - left_fill;

	return text(std::format("{} {} {}",
		(text(std::string(left_fill, 'q')) | line_graphics()).str(),
		str,
		(text(std::string(right_fill, 'q')) | line_graphics()).str()));
}

TermUtils::Text TermUtils::bar(int32_t length)
{
	return bar_text(length);
}

TermUtils::Text TermUtils::status_line(int32_t line, int32_t width, std::string left, std::string right)
{
	std::stringstream ss;
	ss << MOVE_CURSOR_FORMAT(1, line);
	ss << " ";
	ss << left;
	ss << padding(width, left.length(), right.length());
	ss << right;
	ss << " ";
	return text(ss.str()) | reverse_video();
}

TermUtils::Text TermUtils::status_line(int32_t line, int32_t width, const icu::UnicodeString& left, std::string right)
{
	std::string left_out;
	left.toUTF8String(left_out);
	std::stringstream ss;
	ss << MOVE_CURSOR_FORMAT(1, line);
	ss << " ";
	ss << left_out;
	ss << padding(width, static_cast<std::size_t>(left.countChar32()), right.length());
	ss << right;
	ss << " ";
	return text(ss.str()) | reverse_video();
}

TermUtils::Text TermUtils::status_line(int32_t line, int32_t width, const icu::UnicodeString& left, const icu::UnicodeString& right)
{
	std::stringstream ss;
	ss << MOVE_CURSOR_FORMAT(1, line);
	ss << " ";
	ss << left;
	ss << padding(width, static_cast<std::size_t>(left.countChar32()), static_cast<std::size_t>(right.countChar32()));
	ss << right;
	ss << " ";
	return text(ss.str()) | reverse_video();
}

std::string bhm_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, std::string c)
{
	std::stringstream ss;
	int32_t x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
	dx = x2 - x1;
	dy = y2 - y1;
	dx1 = std::abs(dx);
	dy1 = std::abs(dy);
	px = 2 * dy1 - dx1;
	py = 2 * dx1 - dy1;

	if(dy1 <= dx1)
	{
		if(dx >= 0)
		{
			x = x1;
			y = y1;
			xe = x2;
		}
		else
		{
			x = x2;
			y = y2;
			xe = x1;
		}
		ss << TermUtils::pixel(x, y, c);
		for(i = 0; x < xe; i++)
		{
			x = x + 1;
			if(px < 0)
			{
				px = px + 2 * dy1;
			}
			else
			{
			if((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
			{
				y = y + 1;
			}
			else
			{
				y = y - 1;
			}
			px = px + 2 * (dy1 - dx1);
			}
			ss << TermUtils::pixel(x, y, c);
		}
	}
	else
	{
		if(dy >= 0)
		{
			x = x1;
			y = y1;
			ye = y2;
		}
		else
		{
			x = x2;
			y = y2;
			ye = y1;
		}
		ss << TermUtils::pixel(x, y, c);
		for(i = 0; y < ye; i++)
		{
			y = y + 1;
			if(py <= 0)
			{
				py = py + 2 * dx1;
			}
			else
			{
				if((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
				{
					x = x + 1;
				}
				else
				{
					x = x - 1;
				}
				py = py + 2 * (dx1 - dy1);
			}
			ss << TermUtils::pixel(x, y, c);
		}
	}
	return ss.str();
}

TermUtils::Text TermUtils::line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, std::string out)
{
	return text(bhm_line(x1, y1, x2, y2, out));
}

TermUtils::Text TermUtils::frame(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	std::stringstream ss;
	ss << line(x1 + 1, y1, x2 - 1, y1, "q");	// Top H
	ss << line(x1, y1 + 1, x1, y2 - 1, "x");	// Left V
	ss << line(x1 + 1, y2, x2 - 1, y2, "q");	// Bottom H
	ss << line(x2, y1 + 1, x2, y2 - 1, "x");	// Right V
	ss << pixel(x1, y1, "l");
	ss << pixel(x2, y1, "k");
	ss << pixel(x1, y2, "m");
	ss << pixel(x2, y2, "j");
	ss << MOVE_CURSOR_FORMAT(x1 + 1, y1 + 1);
	return text(ss.str()) | line_graphics();
}

TermUtils::Text TermUtils::frame(int32_t x1, int32_t y1, int32_t x2, int32_t y2, std::string msg)
{
	std::stringstream ss;
	ss << frame(x1, y1, x2, y2);
	ss << MOVE_CURSOR_FORMAT(x1 + 3, y1);
	ss << msg;
	ss << MOVE_CURSOR_FORMAT(x1 + 2, y1 + 2);
	return text(ss.str());
}
