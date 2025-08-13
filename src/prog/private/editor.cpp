#include "editor.h"

bool EditorState::set_file(FilePath path, File* f)
{
	if (f == nullptr)
		return false;

	std::string_view view = f->get_view();
	rows_.clear();
	std::ranges::transform(std::views::split(view, '\n'), std::back_inserter(rows_), [](auto v){ return icu::UnicodeString::fromUTF8(v); });
	std::println("Loaded {} row(s).", rows_.size());
	init_state();
	
	path_ = path;
	return true;
}

void EditorState::init_state()
{
	if (rows_.empty())
		rows_.emplace_back(); // Guaranteed one row to write in.

	row_it_ = rows_.end();
	--row_it_; // Move iterator to last line (not end).
	row_ = static_cast<int32_t>((rows_.size() - 1));
	refresh_row();
	move_end();
}

void EditorState::refresh_row()
{
	const icu::UnicodeString& text = row_it_->chars;
	col_it_->setText(text);
	col_it_->first();
	if (col_ = col_it_->next(col_); col_ == icu::BreakIterator::DONE)
	{
		col_ = col_it_->last();
	}
	row_ = static_cast<int32_t>(std::distance(rows_.begin(), row_it_));
	curr_row_bytes_ = text.length(); 
	curr_row_symbols_ = text.countChar32();

	refresh_render();
}

void EditorState::refresh_render()
{
	constexpr int32_t tab_stop_length = 8;

	const icu::UnicodeString& chars = row_it_->chars;
	icu::UnicodeString& render = row_it_->render;

	render.remove();

	if (chars.isEmpty())
		return;

	copy_it_->setText(chars);

	icu::UnicodeString temp{};
	int32_t p1 = 0;
	int32_t p2 = copy_it_->first();
	int32_t render_len = 0;

	while ((p2 = copy_it_->next()) != icu::BreakIterator::DONE)
	{
		int32_t step = p2 - p1;
		std::println("({}-{})", p1, p2);
		chars.extractBetween(p1, p2, temp);

		int32_t next_tab = (render_len == 0) ? tab_stop_length : static_cast<int32_t>(std::ceil(static_cast<float>(render_len) / static_cast<float>(tab_stop_length)) * tab_stop_length);
		int32_t space_count = next_tab - render_len;

		if (temp == '\t')
		{
			std::println("Padding with {} spaces.", space_count);
			render.padTrailing(next_tab, 'X');
			render_len += space_count;
		}
		else
		{
			std::println("Inserting {} chars.", temp.length());
			render.append(temp);
			render_len += step;
		}

		p1 = p2;
	}
}

void EditorState::add_row()
{
	if (!multiline_)
		return;

	if (rows_.empty())
	{
		rows_.emplace_front();
		return;
	}

	icu::UnicodeString& chars = row_it_->chars;

	icu::UnicodeString rest;
	int32_t curr = col_it_->current();
	int32_t len_rest = curr_row_bytes_ - curr;
	chars.extract(curr, len_rest, rest);
	chars.remove(curr, len_rest);
	row_it_ = rows_.emplace(++row_it_, std::move(rest));
	col_ = 0;
	
	refresh_row();
	++dirty_;
}

void EditorState::remove_back()
{
	if (col_ == 0)
	{
		if (row_ > 0 && row_it_ != rows_.begin())
		{
			icu::UnicodeString pop = row_it_->chars;
			row_it_ = rows_.erase(row_it_);
			--row_it_;
			int32_t prev_end = row_it_->chars.countChar32();
			row_it_->chars.append(pop);
			refresh_row();
			move_to(prev_end);
			++dirty_;
		}
		return;
	}

	int32_t curr = col_it_->current();
	col_ = col_it_->previous();
	row_it_->chars.removeBetween(col_, curr);
	refresh_row();
	++dirty_;
}

void EditorState::remove_front()
{
	if (int32_t next = col_it_->next(); next != icu::BreakIterator::DONE)
	{
		row_it_->chars.removeBetween(col_, next);
		++dirty_;
	}
	else if (row_ < (static_cast<int32_t>(rows_.size()) - 1))
	{
		icu::UnicodeString copy = row_it_->chars;
		row_it_ = rows_.erase(row_it_);
		row_it_->chars.insert(0, copy);
		++dirty_;
	}

	/* refresh_row resets the iterator to the correct col_ value, 
	so we don't need to worry about the iterator pos. after next(). */
	refresh_row(); 
}

void EditorState::insert_utf8(const std::string& input)
{
	icu::UnicodeString u_in = icu::UnicodeString::fromUTF8(input);
	int32_t num_points = u_in.countChar32();
	std::println("Inserted {0} code point(s).", num_points);
	row_it_->chars.insert(col_, u_in);
	col_ += num_points;
	refresh_row();
	++dirty_;
}

std::string EditorState::as_utf8() const
{
	return rows_ 
	| std::views::transform([](const EditorRow& u) -> std::string
	{
		std::string out{};
		u.chars.toUTF8String(out);
		return out;
	})
	| std::views::join_with('\n')
	| std::ranges::to<std::string>();
}

EditorState::HandlerReturn EditorState::accept_input(std::string input)
{
	if (input.size() == 0)
		return HandlerReturn::Handled;

	/* Clean erroneous nulls (we should fix this somewhere else). */
	if (input.back() == '\0')
		input.pop_back();

	if (input[0] == '\r')
	{
		add_row();
		return HandlerReturn::Return;
	}

	/* Backspace */
	if (input[0] == '\x7f')
	{
		remove_back();
		return HandlerReturn::Erase;
	}

	if (input[0] == CTRL_KEY('s'))
			return HandlerReturn::WantSave;

	/* Escapes */
	if (input[0] == '\x1b')
	{
		if (input.size() == 1)
			return HandlerReturn::WantExit;

		if (input[1] == '\0')
			return HandlerReturn::WantExit;
		
		if (input[1] == '[' && input.size() >= 3)
		{
			switch(input[2])
			{
				case 'A': move_up(); return HandlerReturn::Handled;
				case 'B': move_down(); return HandlerReturn::Handled;
				case 'C': move_right(); return HandlerReturn::Handled;
				case 'D': move_left(); return HandlerReturn::Handled;
				case 'H': move_home(); return HandlerReturn::Handled;
				case 'F': move_end(); return HandlerReturn::Handled;
				case '3': remove_front(); return HandlerReturn::Handled;
				default: return HandlerReturn::Handled;
			}
		}

		return HandlerReturn::Handled;
	}

	insert_utf8(input);
	return HandlerReturn::PutChar;
}

int32_t EditorState::move_to(int32_t n)
{
	col_it_->first();
	if (col_ = col_it_->next(n); col_ == icu::BreakIterator::DONE)
	{
		col_ = col_it_->last();
	}
	
	return col_;
}

int32_t EditorState::move_up()
{
	if (row_ == 0)
		return 0;

	--row_;
	--row_it_;
	refresh_row();
	return 1;
}

int32_t EditorState::move_up(int32_t count)
{
	for (int32_t i = 0; i < count; ++i)
	{
		if (!move_up())
			return i;
	}
	return count;
}

int32_t EditorState::move_down()
{
	if (row_ == rows_.size() - 1)
		return false;

	++row_;
	++row_it_;
	refresh_row();
	return 1;
}

int32_t EditorState::move_down(int32_t count)
{
	for (int32_t i = 0; i < count; ++i)
	{
		if (!move_down())
			return i;
	}
	return count;
}

int32_t EditorState::move_left()
{
	if (col_ == 0)
		return 0;

	if (col_ = col_it_->previous(); col_ == icu::BreakIterator::DONE)
	{
		col_ = col_it_->first();
		return 0;
	}

	return 1;
}

int32_t EditorState::move_left(int32_t count)
{
	for (int32_t i = 0; i < count; ++i)
	{
		if (!move_left())
			return i;
	}
	return count;
}

int32_t EditorState::move_home()
{
	return move_left(curr_row_bytes_);
}

int32_t EditorState::move_right()
{
	if (col_ == (curr_row_bytes_))
		return 0;

	if (col_ = col_it_->next(); col_ == icu::BreakIterator::DONE)
	{
		col_ = col_it_->last();
		return 0;
	}
	
	return 1;
}

int32_t EditorState::move_right(int32_t count)
{
	for (int32_t i = 0; i < count; ++i)
	{
		if (!move_right())
			return i;
	}
	return count;
}

int32_t EditorState::move_end()
{
	return move_right(curr_row_bytes_);
}

void EditorState::move_cursor(int32_t x, int32_t y)
{
	if (y > 0)
		move_down(y);
	else if (y < 0)
		move_up(std::abs(y));

	if (x > 0)
		move_right(x);
	else if (x < 0)
		move_left(std::abs(x));
}

std::string EditorState::get_printout() const
{
	std::size_t num_rows = rows_.size();

	// This needs to be UnicodeString later (handle non-english filenames).
	std::string left_msg = std::format("{} - {} line{}{}", 
		get_filename(), 
		num_rows, 
		(num_rows != 1 ? "s" : ""), 
		(is_dirty() ? " (modified)" : ""));

	std::string right_msg = std::format("{}/{}[={}] ({}c)", row_ + 1, col_, col_it_->current(), curr_row_bytes_);

	/* Print program chars. */
	std::stringstream ss;
	ss << HIDE_CURSOR CLEAR_SCREEN CLEAR_CURSOR;
	ss << CSI_CODE(33);
	ss << TermUtils::line(1, 1 + num_rows, 1, height_ - 1, "~");
	ss << CSI_RESET;
	ss << TermUtils::status_line(height_, width_, status_.length() ? status_ : icu::UnicodeString::fromUTF8(left_msg), right_msg);
	
	/* Print user chars. */
	ss << CLEAR_CURSOR;
	auto it = rows_.begin();
	// std::advance() this based on scroll pos.
	for (; it != rows_.end(); ++it)
	{
		std::string out_str{};
		it->render.toUTF8String(out_str);
		ss << out_str << "\n";
	}

	ss << MOVE_CURSOR_FORMAT(col_ + 1, row_ + 1);
	ss << SHOW_CURSOR;
	return ss.str();
}
