#include "dbc_win.h"

#include <print>
#include <windows.h>

void DbcWin::set_flags(DWORD& flags, DWORD flag)
{
	flags |= flag;
}

void DbcWin::unset_flags(DWORD& flags, DWORD flag)
{
	flags &= (~flag);
}

bool DbcWin::try_set_flags(DWORD handle, DWORD flags)
{
    HANDLE h = GetStdHandle(handle);
    if (h == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(h, &dwMode))
    {
        return false;
    }

	set_flags(dwMode, flags);

	if (!SetConsoleMode(h, dwMode))
    {
        return false;
    }

	return true;
}

bool DbcWin::try_unset_flags(DWORD handle, DWORD flags)
{
    HANDLE h = GetStdHandle(handle);
    if (h == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(h, &dwMode))
    {
        return false;
    }

	unset_flags(dwMode, flags);

	if (!SetConsoleMode(h, dwMode))
    {
        return false;
    }

	return true;
}

bool DbcWin::enable_vtt_mode()
{
	bool i = try_set_flags(STD_INPUT_HANDLE, ENABLE_VIRTUAL_TERMINAL_INPUT);
	bool o = try_set_flags(STD_OUTPUT_HANDLE, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	return i && o;
}

bool DbcWin::disable_vtt_mode()
{
	bool i = try_unset_flags(STD_INPUT_HANDLE, ENABLE_VIRTUAL_TERMINAL_INPUT);
	bool o = try_unset_flags(STD_OUTPUT_HANDLE, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	return i && o;
}

bool DbcWin::enable_raw_mode()
{
	return try_unset_flags(STD_INPUT_HANDLE, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
}

bool DbcWin::disable_raw_mode()
{
	return try_set_flags(STD_INPUT_HANDLE, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
}

std::string DbcWin::getch()
{
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	if (h == NULL) 
	{
		std::println("No console!");
		return {}; // console not found
	}
	DWORD count;
	TCHAR c[1024];
	ReadConsole(h, c, 1024, &count, NULL);
	std::string out;
	out.assign(c, count);
	return out;
}

/* Read Unicode (UTF-16) input from console */
std::wstring DbcWin::read_console_input_w() 
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    std::wstring result;
    wchar_t buffer[256];
    DWORD charsRead;

    if (ReadConsoleW(hInput, buffer, 255, &charsRead, nullptr))
	{
        buffer[charsRead] = L'\0';  // Null-terminate
        result = buffer;
    }
    return result;
}

std::string DbcWin::utf16_to_utf8(const std::wstring& wstr) 
{
    if (wstr.empty()) return std::string();

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
                                         nullptr, 0, nullptr, nullptr);

    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
                        &strTo[0], sizeNeeded, nullptr, nullptr);

    return strTo;
}