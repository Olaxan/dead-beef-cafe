#include "term_utils.h"

#include <sstream>

int32_t TermUtils::get_ansi_fg_color(TermColor color)
{
	switch (color)
	{
		case TermColor::Black: 			return 30;
		case TermColor::Red: 			return 31;
		case TermColor::Green:   		return 32;
		case TermColor::Yellow:  		return 33;
		case TermColor::Blue:    		return 34;
		case TermColor::Magenta: 		return 35;
		case TermColor::Cyan:    		return 36;
		case TermColor::White:   		return 37;
		case TermColor::BrightBlack:    return 90;
		case TermColor::BrightRed:      return 91;
		case TermColor::BrightGreen:    return 92;
		case TermColor::BrightYellow:   return 93;
		case TermColor::BrightBlue:     return 94;
		case TermColor::BrightMagenta:  return 95;
		case TermColor::BrightCyan:     return 96;
		case TermColor::BrightWhite:    return 97;
		default: return 37;
	}
}

int32_t TermUtils::get_ansi_bg_color(TermColor color)
{
	switch (color)
	{
		case TermColor::Black: 			return 40;
		case TermColor::Red: 			return 41;
		case TermColor::Green:   		return 42;
		case TermColor::Yellow:  		return 43;
		case TermColor::Blue:    		return 44;
		case TermColor::Magenta: 		return 45;
		case TermColor::Cyan:    		return 46;
		case TermColor::White:   		return 47;
		case TermColor::BrightBlack:    return 100;
		case TermColor::BrightRed:      return 101;
		case TermColor::BrightGreen:    return 102;
		case TermColor::BrightYellow:   return 103;
		case TermColor::BrightBlue:     return 104;
		case TermColor::BrightMagenta:  return 105;
		case TermColor::BrightCyan:     return 106;
		case TermColor::BrightWhite:    return 107;
		default: return 40;
	}
}

std::string TermUtils::color(const std::string& str, TermColor fg, TermColor bg)
{
	return std::format(CSI "{};{}m{}" CSI_CODE(0), TermUtils::get_ansi_fg_color(fg), TermUtils::get_ansi_bg_color(bg), str);
}

std::string TermUtils::pixel(int32_t x, int32_t y, std::string c)
{
	return std::format(MOVE_CURSOR_PLACEHOLDER "{}", y, x, c);
}

std::string TermUtils::line(int32_t length)
{
	return std::format(LINE_BEGIN "{}" LINE_END, std::string(length, 'q'));
}

std::string TermUtils::msg_line(std::string str, int32_t length)
{
	return std::format("{} " LINE_BEGIN "{}" LINE_END, str, std::string(length - 1, 'q'));
}

std::string TermUtils::status_line(int32_t line, int32_t width, std::string left, std::string right)
{
	std::stringstream ss;
	ss << REVERSE_VIDEO;
	ss << MOVE_CURSOR_FORMAT(1, line);
	ss << " ";
	ss << left;
	ss << std::string(width - (left.length() + right.length() + 2), ' ');
	ss << right;
	ss << " ";
	ss << CSI_RESET;
	return ss.str();
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

std::string TermUtils::line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, std::string out)
{
	return bhm_line(x1, y1, x2, y2, out);
}

std::string TermUtils::frame(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	std::stringstream ss;
	ss << LINE_BEGIN; 					// Enter line drawing mode
	ss << line(x1 + 1, y1, x2 - 1, y1, "q");	// Top H
	ss << line(x1, y1 + 1, x1, y2 - 1, "x");	// Left V
	ss << line(x1 + 1, y2, x2 - 1, y2, "q");	// Bottom H
	ss << line(x2, y1 + 1, x2, y2 - 1, "x");	// Right V
	ss << pixel(x1, y1, "l");
	ss << pixel(x2, y1, "k");
	ss << pixel(x1, y2, "m");
	ss << pixel(x2, y2, "j");
	ss << MOVE_CURSOR_FORMAT(x1 + 1, y1 + 1);
    ss << LINE_END; 					// Exit line drawing mode
	return ss.str();
}

std::string TermUtils::frame(int32_t x1, int32_t y1, int32_t x2, int32_t y2, std::string msg)
{
	std::stringstream ss;
	ss << frame(x1, y1, x2, y2);
	ss << MOVE_CURSOR_FORMAT(x1 + 3, y1);
	ss << msg;
	ss << MOVE_CURSOR_FORMAT(x1 + 2, y1 + 2);
	return ss.str();
}
