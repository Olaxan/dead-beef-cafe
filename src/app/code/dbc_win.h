#pragma once

#include <string>

#define DBC_DWORD unsigned long

namespace DbcWin
{
	void set_flags(DBC_DWORD& flags, DBC_DWORD flag);
	void unset_flags(DBC_DWORD& flags, DBC_DWORD flag);
	bool try_set_flags(DBC_DWORD handle, DBC_DWORD flags);
	bool try_unset_flags(DBC_DWORD handle, DBC_DWORD flags);

	bool enable_vtt_mode();
	bool disable_vtt_mode();
	bool enable_raw_mode();
	bool disable_raw_mode();

	std::string getch();

	/* Read Unicode (UTF-16) input from console */
	std::wstring read_console_input_w();

	std::string utf16_to_utf8(const std::wstring& wstr);

};

#undef DBC_DWORD