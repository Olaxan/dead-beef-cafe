#include "term_utils.h"

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
