#pragma once

#include "filesystem.h"

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/usearch.h>
#include <unicode/ustring.h>
#include <unicode/ustream.h>
#include <unicode/brkiter.h>

#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <cctype>

class EditorState
{
public:

	enum class HandlerReturn
	{
		Handled = 0,
		Unhandled,
		PutChar,
		Return,
		Erase,
		WantSave,
		WantExit
	};

	EditorState() = delete;

	EditorState(int32_t width, int32_t height, bool multiline = true)
	: width_(width), height_(height), multiline_(multiline), error_(U_ZERO_ERROR), col_it_(icu::BreakIterator::createCharacterInstance(icu::Locale::getDefault(), error_))
	{
		init_state();
	}

	std::size_t get_num_rows() const { return rows_.size(); }
	int32_t get_col() const { return col_; }
	int32_t get_row() const { return row_; }
	std::string_view get_filename() const { return has_file() ? path_->get_name() : "*new file*"; }
	bool is_dirty() const { return dirty_ > 0; }
	bool has_file() const { return path_.has_value(); }
	std::optional<FilePath> get_path() const { return path_; }
	void set_dirty(int32_t new_dirty) { dirty_ = new_dirty; }
	void set_path(FilePath new_path) { path_ = std::move(new_path); }

	bool set_file(FilePath path, File* f);

	void init_state();

	void set_size(int32_t width, int32_t height)
	{
		width_ = width;
		height_ = height;
	}

	void set_status(std::string new_status)
	{
		status_ = icu::UnicodeString::fromUTF8(std::move(new_status));
	}

	void set_status(const char* new_status)
	{
		status_ = icu::UnicodeString(new_status);
	}

	void reset_status() { status_.remove(); }

	bool refresh_row();

	void add_row();
	void remove_back();
	void remove_front();

	void insert_utf8(const std::string& input);

	std::string as_utf8() const;

	HandlerReturn accept_input(std::string input);

	int32_t move_to(int32_t n);
	int32_t move_up();
	int32_t move_up(int32_t count);
	int32_t move_down();
	int32_t move_down(int32_t count);
	int32_t move_left();
	int32_t move_left(int32_t count);
	int32_t move_home();
	int32_t move_right();
	int32_t move_right(int32_t count);
	int32_t move_end();
	void move_cursor(int32_t x, int32_t y);

	/* Get a ANSI string to print this to console.
	This should be made replaceable (it is not the editor's job). */
	std::string get_printout() const;

protected:

	int32_t row_{0};
	int32_t col_{0};
	int32_t width_{1};
	int32_t height_{1};
	int32_t dirty_ {0};
	int32_t curr_row_len_{0};
	bool multiline_{true};
	icu::UnicodeString status_{};
	std::list<icu::UnicodeString> rows_{};
	std::list<icu::UnicodeString>::iterator row_it_{};
	UErrorCode error_{U_ZERO_ERROR};
	std::unique_ptr<icu::BreakIterator> col_it_{nullptr};
	std::optional<FilePath> path_{};
};