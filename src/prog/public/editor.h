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

	struct EditorRow
	{
		EditorRow() = default;

		EditorRow(const icu::UnicodeString& other)
			: chars(other) {};

		icu::UnicodeString chars{};
		icu::UnicodeString render{};
	};

	EditorState() = delete;

	EditorState(int32_t width, int32_t height, bool multiline = true)
		: width_(width), height_(height), multiline_(multiline), 
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

	/* Get the filename of the current file, or a default if the file has never been saved. */
	std::string_view get_filename() const { return has_file() ? path_->get_name() : "*new file*"; }

	/* Returns whether the current file has had modifications done since the last save. */
	bool is_dirty() const { return dirty_ > 0; }

	/* Return whether this file has ever been saved (has a file path). */
	bool has_file() const { return path_.has_value(); }

	/* Returns the last-saved-to file path, if the file has been saved, or nullopt otherwise. */
	std::optional<FilePath> get_path() const { return path_; }

	/* Set the new dirtiness state of the file (number of modifications since last save). */
	void set_dirty(int32_t new_dirty) { dirty_ = new_dirty; }

	/* Set the save-to filepath of the file. */
	void set_path(FilePath new_path) { path_ = std::move(new_path); }

	/* Open a file and load some text from it. */
	bool set_file(FilePath path, File* f);

	/* Initialise the state of the editor after it has been constructed,
	or a new file has been loaded. Ensures the editor is in a valid state,
	i.e. has a row for typing, aligns iterators, etc. */
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
	void insert_utf8(const std::string& input);

	/* Get the contents of the buffer as a utf-8 formatted string. */
	std::string as_utf8() const;

	/* Take some text from the input stream and process it. */
	HandlerReturn accept_input(std::string input);

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

	/* Get a ANSI string to print this to console.
	This should be made replaceable (it is not the editor's job). */
	std::string get_printout() const;

protected:

	int32_t row_{0};
	int32_t col_{0};
	int32_t width_{1};
	int32_t height_{1};
	int32_t dirty_ {0};
	int32_t curr_row_bytes_{0};
	int32_t curr_row_symbols_{0};
	bool multiline_{true};
	icu::UnicodeString status_{};
	std::list<EditorRow> rows_{};
	std::list<EditorRow>::iterator row_it_{};
	std::unique_ptr<icu::BreakIterator> col_it_{nullptr};
	std::unique_ptr<icu::BreakIterator> copy_it_{nullptr};
	UErrorCode error_{U_ZERO_ERROR};
	std::optional<FilePath> path_{};

};