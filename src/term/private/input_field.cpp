#include "input_field.h"

#include "term_utils.h"

#include <ranges>
#include <algorithm>

bool InputField::set_text(std::string_view view)
{
	rows_.clear();
	std::ranges::transform(std::views::split(view, '\n'), std::back_inserter(rows_), [](auto v){ return icu::UnicodeString::fromUTF8(v); });
	init_state();
	
	return true;
}

void InputField::init_state()
{
	if (rows_.empty())
		rows_.emplace_back(); // Guaranteed one row to write in.

	row_it_ = rows_.begin();
	for (; row_it_ != rows_.end(); ++row_it_)
	{
		refresh_row();
	}

	--row_it_; // Move iterator to last line (not end).
	row_ = static_cast<int32_t>((rows_.size() - 1));
	move_end();
}

void InputField::refresh_row()
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

void InputField::refresh_render()
{
	constexpr int32_t tab_stop_length = 4;

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
		int32_t next_tab = (render_len == 0) ? tab_stop_length : static_cast<int32_t>(std::ceil(static_cast<float>(render_len + 1) / static_cast<float>(tab_stop_length)) * tab_stop_length);
		int32_t space_count = next_tab - render_len;

		if (chars.char32At(p1) == '\t')
		{
			render.padTrailing(next_tab);
			render_len += space_count;
		}
		else
		{
			chars.extractBetween(p1, p2, temp);
			render.append(temp);
			render_len += step;
		}

		p1 = p2;
	}
}

int32_t InputField::get_adjusted_col() const
{
	constexpr int32_t tab_stop_length = 4;

	const icu::UnicodeString& chars = row_it_->chars;
	const icu::UnicodeString& render = row_it_->render;

	if (chars.isEmpty())
		return 0;

	copy_it_->setText(chars);

	int32_t adj = 0;
	int32_t p1 = 0;
	int32_t p2 = copy_it_->first();

	while ((p2 = copy_it_->next()) != icu::BreakIterator::DONE)
	{
		if (p2 > col_)
			break;

		if (chars.char32At(p1) == '\t')
		{
			adj += (tab_stop_length - 1) - (adj % tab_stop_length);
		}

		int32_t guess = get_approx_point_width(chars.char32At(p1));
		int32_t step = p2 - p1;

		adj += std::max(step, guess);
		p1 = p2;
	}

	return adj;
}

int32_t InputField::get_approx_point_width(char32_t ch) const
{
	// Combining marks occupy no columns.
    if (u_hasBinaryProperty(ch, UCHAR_GRAPHEME_EXTEND))
        return 0;

    // Control characters.
    if (u_charType(ch) == U_CONTROL_CHAR)
        return 0;

    switch (static_cast<UEastAsianWidth>(u_getIntPropertyValue(ch, UCHAR_EAST_ASIAN_WIDTH)))
    {
        case U_EA_FULLWIDTH:
        case U_EA_WIDE:
            return 2;

        default:
            return 1;
    }
}

void InputField::add_row()
{
	if (!params_.multiline)
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

void InputField::remove_back()
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

void InputField::remove_front()
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

void InputField::insert_utf8(std::string_view input)
{
	icu::UnicodeString u_in = icu::UnicodeString::fromUTF8(input);
	int32_t num_points = u_in.countChar32();
	//std::println("Inserted {0} code point(s).", num_points);
	row_it_->chars.insert(col_, u_in);
	col_ += num_points;
	refresh_row();
	++dirty_;
}

std::string InputField::as_utf8() const
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

std::string InputField::render_line_utf8(bool unescape) const
{
	std::string writeback;
	const icu::UnicodeString& chars = row_it_->render;

	if (unescape)
	{
		icu::UnicodeString unesc = chars.unescape();
		unesc.toUTF8String(writeback);
	}
	else
	{
		chars.toUTF8String(writeback);
	}
	
	return writeback;
}

size_t InputField::current_line_length() const
{
	const icu::UnicodeString& chars = row_it_->chars;
	return chars.countChar32();
}

InputField::HandlerReturn InputField::accept_input(std::string_view input)
{
	if (input.size() == 0)
		return HandlerReturn::Handled;

	/* Clean erroneous nulls (we should fix this somewhere else). */
	if (input.back() == '\0')
		input.remove_suffix(1);

	if (input.size() == 0)
		return HandlerReturn::Handled;

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

int32_t InputField::move_to(int32_t n)
{
	col_it_->first();
	if (col_ = col_it_->next(n); col_ == icu::BreakIterator::DONE)
	{
		col_ = col_it_->last();
	}
	
	return col_;
}

int32_t InputField::move_up()
{
	if (row_ == 0)
		return 0;

	--row_;
	--row_it_;
	refresh_row();
	return 1;
}

int32_t InputField::move_up(int32_t count)
{
	for (int32_t i = 0; i < count; ++i)
	{
		if (!move_up())
			return i;
	}
	return count;
}

int32_t InputField::move_down()
{
	if (row_ == rows_.size() - 1)
		return false;

	++row_;
	++row_it_;
	refresh_row();
	return 1;
}

int32_t InputField::move_down(int32_t count)
{
	for (int32_t i = 0; i < count; ++i)
	{
		if (!move_down())
			return i;
	}
	return count;
}

int32_t InputField::move_left()
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

int32_t InputField::move_left(int32_t count)
{
	for (int32_t i = 0; i < count; ++i)
	{
		if (!move_left())
			return i;
	}
	return count;
}

int32_t InputField::move_home()
{
	return move_left(curr_row_bytes_);
}

int32_t InputField::move_right()
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

int32_t InputField::move_right(int32_t count)
{
	for (int32_t i = 0; i < count; ++i)
	{
		if (!move_right())
			return i;
	}
	return count;
}

int32_t InputField::move_end()
{
	return move_right(curr_row_bytes_);
}

void InputField::move_cursor(int32_t x, int32_t y)
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
