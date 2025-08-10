#include "os_basic.h"

#include "filesystem.h"
#include "navigator.h"

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

	bool set_file(FilePath path, File* f)
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

	void init_state()
	{
		if (rows_.empty())
			rows_.emplace_back(); // Guaranteed one row to write in.

		row_it_ = rows_.end();
		--row_it_; // Move iterator to last line (not end).
		row_ = static_cast<int32_t>((rows_.size() - 1));
		refresh_row();
	}

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

	bool refresh_row()
	{
		const icu::UnicodeString& text = *row_it_;
		col_it_->setText(text);
		col_it_->first();
		if (col_ = col_it_->next(col_); col_ == icu::BreakIterator::DONE)
		{
			col_ = col_it_->last();
		}
		row_ = static_cast<int32_t>(std::distance(rows_.begin(), row_it_));
		curr_row_len_ = text.length(); //text.countChar32();

		return true;
	}

	void add_row()
	{

		if (!multiline_)
			return;

		if (rows_.empty())
		{
			rows_.emplace_front();
			return;
		}

		icu::UnicodeString rest;
		int32_t curr = col_it_->current();
		int32_t len_rest = curr_row_len_ - curr;
		row_it_->extract(curr, len_rest, rest);
		row_it_->remove(curr, len_rest);
		row_it_ = rows_.emplace(++row_it_, std::move(rest));
		col_ = 0;
		
		refresh_row();
		++dirty_;
	}

	void remove_back()
	{
		if (col_ == 0)
		{
			if (row_ > 0 && row_it_ != rows_.begin())
			{
				icu::UnicodeString pop = *row_it_;
				row_it_ = rows_.erase(row_it_);
				--row_it_;
				int32_t prev_end = row_it_->countChar32();
				row_it_->append(pop);
				refresh_row();
				move_to(prev_end);
				++dirty_;
			}
			return;
		}

		int32_t curr = col_it_->current();
		col_ = col_it_->previous();
		row_it_->removeBetween(col_, curr);
		refresh_row();
		++dirty_;
	}

	void remove_front()
	{
		if (int32_t next = col_it_->next(); next != icu::BreakIterator::DONE)
		{
			row_it_->removeBetween(col_, next);
			++dirty_;
		}
		else if (row_ < (static_cast<int32_t>(rows_.size()) - 1))
		{
			icu::UnicodeString copy = *row_it_;
			row_it_ = rows_.erase(row_it_);
			row_it_->insert(0, copy);
			++dirty_;
		}

		/* refresh_row resets the iterator to the correct col_ value, 
		so we don't need to worry about the iterator pos. after next(). */
		refresh_row(); 
	}

	void insert_utf8(const std::string& input)
	{
		icu::UnicodeString u_in = icu::UnicodeString::fromUTF8(input);
		int32_t num_points = u_in.countChar32();
		std::println("Inserted {0} code point(s).", num_points);
		row_it_->insert(col_, u_in);
		col_ += num_points;
		refresh_row();
		++dirty_;
	}

	std::string as_utf8() const
	{
		return rows_ 
		| std::views::transform([](const icu::UnicodeString& u) -> std::string
		{
			std::string out{};
			u.toUTF8String(out);
			return out;
		})
		| std::views::join_with('\n')
		| std::ranges::to<std::string>();
	}

	HandlerReturn accept_input(std::string input)
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

	int32_t move_to(int32_t n)
	{
		col_it_->first();
		if (col_ = col_it_->next(n); col_ == icu::BreakIterator::DONE)
		{
			col_ = col_it_->last();
		}
		
		return col_;
	}

	int32_t move_up()
	{
		if (row_ == 0)
			return 0;

		--row_;
		--row_it_;
		refresh_row();
		return 1;
	}

	int32_t move_up(int32_t count)
	{
		for (int32_t i = 0; i < count; ++i)
		{
			if (!move_up())
				return i;
		}
		return count;
	}

	int32_t move_down()
	{
		if (row_ == rows_.size() - 1)
			return false;

		++row_;
		++row_it_;
		refresh_row();
		return 1;
	}

	int32_t move_down(int32_t count)
	{
		for (int32_t i = 0; i < count; ++i)
		{
			if (!move_down())
				return i;
		}
		return count;
	}

	int32_t move_left()
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

	int32_t move_left(int32_t count)
	{
		for (int32_t i = 0; i < count; ++i)
		{
			if (!move_left())
				return i;
		}
		return count;
	}

	int32_t move_home()
	{
		return move_left(curr_row_len_);
	}

	int32_t move_right()
	{
		if (col_ == (curr_row_len_))
			return 0;

		if (col_ = col_it_->next(); col_ == icu::BreakIterator::DONE)
		{
			col_ = col_it_->last();
			return 0;
		}
		
		return 1;
	}

	int32_t move_right(int32_t count)
	{
		for (int32_t i = 0; i < count; ++i)
		{
			if (!move_right())
				return i;
		}
		return count;
	}

	int32_t move_end()
	{
		return move_right(curr_row_len_);
	}

	void move_cursor(int32_t x, int32_t y)
	{
		if (y > 0)
			move_down(y);
		else if (y < 0)
			move_up(std::abs(y));

		if (x > 0)
			move_right(x);
		else if (x < 0)
			move_left(std::abs(x));
	};

	/* Get a ANSI string to print this to console.
	This should be made replaceable (it is not the editor's job). */
	std::string get_printout() const
	{
		std::size_t num_rows = rows_.size();

		// This needs to be UnicodeString later (handle non-english filenames).
		std::string left_msg = std::format("{} - {} line{}{}", 
			get_filename(), 
			num_rows, 
			(num_rows != 1 ? "s" : ""), 
			(is_dirty() ? " (modified)" : ""));

		std::string right_msg = std::format("{}/{}[={}] ({}c)", row_ + 1, col_, col_it_->current(), curr_row_len_);

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
			it->toUTF8String(out_str);
			ss << out_str << "\n";
		}

		ss << MOVE_CURSOR_FORMAT(col_ + 1, row_ + 1);
		ss << SHOW_CURSOR;
		return ss.str();
	}

protected:

	int32_t row_ = 0;
	int32_t col_ = 0;
	int32_t width_ = 1;
	int32_t height_ = 1;
	int32_t dirty_ = 0;
	int32_t curr_row_len_ = 0;
	bool multiline_ = true;
	icu::UnicodeString status_{};
	std::list<icu::UnicodeString> rows_{};
	std::list<icu::UnicodeString>::iterator row_it_{};
	UErrorCode error_ = U_ZERO_ERROR;
	std::unique_ptr<icu::BreakIterator> col_it_{nullptr};
	std::optional<FilePath> path_{};
};


EagerTask<com::CommandQuery> await_input(Proc& proc)
{
	OS& os = *proc.owning_os;

	if (auto async_read = proc.read<CmdSocketAwaiterServer>())
	{
		co_return (co_await *async_read);
	}
	else
	{
		while (true)
		{
			if (std::optional<com::CommandQuery> opt_input = proc.read<com::CommandQuery>())
				co_return *opt_input;

			co_await os.wait(0.16f);
		}
	}
}

EagerTask<std::optional<std::string>> write_in_statusbar(EditorState& state, Proc& proc, std::string prefix)
{
	OS& os = *proc.owning_os;

	EditorState substate{25, 1, false};
	substate.init_state();

	state.set_status(std::format("{}: {}", prefix, substate.as_utf8()));
	proc.put("{}", state.get_printout());

	while (true)
	{
		com::CommandQuery query_in = co_await await_input(proc);
		std::string str_in = query_in.command();
		EditorState::HandlerReturn ret = substate.accept_input(str_in);

		if (ret == EditorState::HandlerReturn::WantExit)
		{
			state.set_status("Save aborted.");
			co_return std::nullopt;
		}

		if (ret == EditorState::HandlerReturn::Return)
		{
			break;
		}

		state.set_status(std::format("{}: {}", prefix, substate.as_utf8()));
		proc.put("{}", state.get_printout());
	}

	state.reset_status();
	co_return substate.as_utf8();
}

EagerTask<bool> handle_save(EditorState& state, Proc& proc);
EagerTask<bool> handle_save_as(EditorState& state, Proc& proc)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();

	if (std::optional<std::string> name = co_await write_in_statusbar(state, proc, "Save as"))
	{
		FilePath new_path(*name);
		if (new_path.is_relative())
			new_path.prepend(proc.get_var("SHELL_PATH"));

		state.set_path(new_path);

		co_return (co_await handle_save(state, proc));
	}

	co_return false;
}

EagerTask<bool> handle_save(EditorState& state, Proc& proc)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();

	if (auto opt_path = state.get_path(); opt_path.has_value())
	{
		auto [file, err] = fs->open(*opt_path, FileAccessFlags::Create);
		if (err == FileSystemError::Success)
		{
			file->write(state.as_utf8());
			state.set_dirty(0);
			co_return true;
		}
	}

	co_return (co_await handle_save_as(state, proc));

}

ProcessTask Programs::CmdEdit(Proc& proc, std::vector<std::string> args)
{
	OS& os = *proc.owning_os;
	FileSystem* fs = os.get_filesystem();

	if (fs == nullptr)
	{
		proc.errln("No filesystem!");
		co_return 1;
	}

	int32_t term_w = proc.get_var<int32_t>("TERM_W");
	int32_t term_h = proc.get_var<int32_t>("TERM_H");

	if (term_w < 20 || term_h < 10)
	{
		proc.warnln("The terminal window is too small, or 'TERM_W'/'TERM_H' wasn't set.");
		co_return 1;
	}

	EditorState state{term_w, term_h};

	if (args.size() > 1)
	{
		FilePath path(args[1]);

		if (path.is_relative())
			path.prepend(proc.get_var("SHELL_PATH"));

		auto [f, err] = fs->open(path, FileAccessFlags::Create);
		if (f && err == FileSystemError::Success)
		{
			state.set_file(path, f);
		}

		std::println("Opening {}: {}.", path, FileSystem::get_fserror_name(err));
	}

	com::CommandReply begin_msg;
	begin_msg.set_reply(std::format(BEGIN_ALT_SCREEN_BUFFER "{}", state.get_printout()));
	proc.write(begin_msg);

	while (true)
	{
		com::CommandQuery input = co_await await_input(proc);

		/* First, update terminal parameters if we're being passed configuration data. */
		if (input.has_screen_data())
		{
			com::ScreenData screen = input.screen_data();
			state.set_size(screen.size_x(), screen.size_y());
		}

		/* Next, read the actual command and consider it based on first-byte. */
		std::string str_in = input.command();

		for (char c : str_in)
			std::print("{}, ", (int)c);
		std::println("was received.");

		EditorState::HandlerReturn ret = state.accept_input(str_in);

		if (ret == EditorState::HandlerReturn::WantExit)
		{
			if (!state.is_dirty())
				break;

			state.set_status("The file is unsaved (Ctrl+S to save).");
		}

		if (ret == EditorState::HandlerReturn::WantSave)
		{
			co_await handle_save(state, proc);
		}		

		proc.put("{}", state.get_printout());
	}

	com::CommandReply end_msg;
	end_msg.set_reply(END_ALT_SCREEN_BUFFER "Goodbye...\n");
	proc.write(end_msg);

	co_return 0;
}
