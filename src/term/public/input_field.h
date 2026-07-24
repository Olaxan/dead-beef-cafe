#pragma once

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/usearch.h>
#include <unicode/ustring.h>
#include <unicode/ustream.h>
#include <unicode/brkiter.h>

#include <list>
#include <string>
#include <vector>
#include <print>
#include <chrono>
#include <format>
#include <cctype>
#include <string_view>

struct InputFieldParams
{
	/* Whether we allow line breaks. */
	bool multiline{true};
};

class InputField 
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

	struct EditorRow
	{
		EditorRow() = default;

		EditorRow(const icu::UnicodeString& other)
			: chars(other) {};

		icu::UnicodeString chars{};
		icu::UnicodeString render{};
	};

	InputField() = delete;

	InputField(InputFieldParams&& params)
		: params_(std::move(params)), 
		col_it_(icu::BreakIterator::createCharacterInstance(icu::Locale::getDefault(), error_)),
		copy_it_(icu::BreakIterator::createCharacterInstance(icu::Locale::getDefault(), error_))
	{
		init_state();
	}

	/* Return the number of filled editor rows. */
	std::size_t get_num_rows() const { return rows_.size(); }

	/* Return the cursor X position. */
	int32_t get_col() const { return col_; }

	/* Return the cursor Y position. */
	int32_t get_row() const { return row_; }

	/* Returns the column index (Y), adjusted for tab stops. */
	int32_t get_adjusted_col() const;

	/* Get a best-guess heuristic of how many columns a unicode character will take in the terminal. */
	int32_t get_approx_point_width(char32_t point) const;

	/* Initialise the state of the editor after it has been constructed,
	or a new file has been loaded. Ensures the editor is in a valid state,
	i.e. has a row for typing, aligns iterators, etc. */
	void init_state();

	/* Re-initialise the input field from an existing string.
	Expected to be encoded in UTF-8 and handles Unicode. */
	bool set_text(std::string_view input);

	/* This important function must be called when a operation has been performed,
	and refreshes the state of the *current* row. */
	void refresh_row();

	/* Refreshes the render string -- called by refresh_row. */
	void refresh_render();

	/* Add a linebreak at the current cursor position,
	breaking the text in twain if needed. */
	void add_row();

	/* Remove one unicode character, erasing backwards (backspace key). */
	void remove_back();

	/* Remove one unicode character, erasing forwards (delete key). */
	void remove_front();

	/* Insert some text into the current cursor position,
	formatted as utf-8. */
	void insert_utf8(std::string_view input);

	/* Get the contents of the buffer as a utf-8 formatted string. */
	std::string as_utf8() const;

	/* Get the current line as a utf-8 formatted string. */
	std::string render_line_utf8(bool unescape = true) const;

	/* Get the length (printable chars) of the current line. */
	size_t render_line_length() const;

	/* Take some text from the input stream and process it. */
	HandlerReturn accept_input(std::string_view input);

	/* --- Cursor movement functions --- */

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

protected:

	int32_t row_{0};
	int32_t col_{0};
	int32_t curr_row_bytes_{0};
	int32_t curr_row_symbols_{0};
	int32_t dirty_ {0};
	
	InputFieldParams params_{};

	std::list<EditorRow> rows_{};
	std::list<EditorRow>::iterator row_it_{};
	std::unique_ptr<icu::BreakIterator> col_it_{nullptr};
	std::unique_ptr<icu::BreakIterator> copy_it_{nullptr};
	UErrorCode error_{U_ZERO_ERROR};

};