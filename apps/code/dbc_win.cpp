#include "dbc_win.h"

#include <print>
#include <vector>
#include <sstream>
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

std::string DbcWin::fetch_terminal_events()
{
	auto get_input_records = [&]() -> std::vector<INPUT_RECORD>
	{
		// Check if there is input in the console.
		auto console = GetStdHandle(STD_INPUT_HANDLE);
		DWORD number_of_events = 0;
		if (!GetNumberOfConsoleInputEvents(console, &number_of_events)) 
		{
			return std::vector<INPUT_RECORD>();
		}
		if (number_of_events <= 0) 
		{
			// No input, return.
			return std::vector<INPUT_RECORD>();
		}
		// Read the input events.
		std::vector<INPUT_RECORD> records(number_of_events);
		DWORD number_of_events_read = 0;
		if (!ReadConsoleInput(console, records.data(), (DWORD)records.size(),
			&number_of_events_read)) 
		{
			return std::vector<INPUT_RECORD>();
		}
		records.resize(number_of_events_read);
		return records;
	};

	auto records = get_input_records();
	if (records.size() == 0) 
	{
		return std::string();
	}

	// Convert the input events to FTXUI events.
	// For each event, we call the terminal input parser to convert it to
	// Event.
	std::wstring wstring;
	std::stringstream ss;
	for (const auto& r : records) 
	{
		switch (r.EventType) 
		{
		case KEY_EVENT: 
		{
			auto key_event = r.Event.KeyEvent;
			// ignore UP key events
			if (key_event.bKeyDown == FALSE) 
			{
				continue;
			}
			const wchar_t wc = key_event.uChar.UnicodeChar;
			wstring += wc;
			if (wc >= 0xd800 && wc <= 0xdbff) {
				// Wait for the Low Surrogate to arrive in the next record.
				continue;
			}
			ss << utf16_to_utf8(wstring);
			wstring.clear();
		} break;
		case WINDOW_BUFFER_SIZE_EVENT:
			ss << "\033[8;" << r.Event.WindowBufferSizeEvent.dwSize.Y << ";" << r.Event.WindowBufferSizeEvent.dwSize.X << "t";
			break;
		case MENU_EVENT:
		case FOCUS_EVENT:
		case MOUSE_EVENT:
			break;
		}
	}
	return ss.str();
}