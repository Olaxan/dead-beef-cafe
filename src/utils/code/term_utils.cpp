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

std::string TermUtils::line(int32_t length)
{
	return std::format(LINE_BEGIN "{}" LINE_END, std::string(length, 'q'));
}

std::string TermUtils::msg_line(std::string str, int32_t length)
{
	return std::format("{} " LINE_BEGIN "{}" LINE_END, str, std::string(length - 1, 'q'));
}

std::string TermUtils::line_vertical_border()
{
	std::stringstream ss;
    ss << ESC "(0"; 		// Enter Line drawing mode
    ss << CSI "104;93m"; 	// bright yellow on bright blue
    ss << "x"; 				// in line drawing mode, \x78 -> \u2502 "Vertical Bar"
    ss << CSI "0m";			// restore color
    ss << ESC "(B"; 		// exit line drawing mode
	return ss.str();
}

std::string TermUtils::line_horizontal_border(int32_t width, bool fIsTop)
{
	std::stringstream ss;
    ss << ESC "(0"; 			// Enter Line drawing mode
    ss << CSI "104;93m"; 		// Make the border bright yellow on bright blue
    ss << fIsTop ? "l" : "m"; 	// print left corner 

    for (int32_t i = 1; i < width - 1; i++)
        ss << "q"; 				// in line drawing mode, \x71 -> \u2500 "HORIZONTAL SCAN LINE-5"

    ss << fIsTop ? "k" : "j"; 	// print right corner
    ss << CSI "0m";
    ss << ESC "(B"; 			// exit line drawing mode
	return ss.str();
}

std::string TermUtils::frame(int32_t width, int32_t height)
{
	std::stringstream ss;
	ss << CSI << "2;1H";
	ss << line_horizontal_border(width, true);
	ss << CSI << std::format("{};1H", height - 1);
	ss << line_horizontal_border(width, true);
	return ss.str();
}
