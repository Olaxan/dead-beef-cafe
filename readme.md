# Dead Beef Cafe

A terminal based MP hacking game.

## Terminal text utilities

Terminal text effects can be composed with `|`. Each effect closes its own
terminal mode after the text, so foreground/background colors and attributes
do not leak into following output:

```cpp
using namespace TermUtils;

std::string label = std::format("{}", text("Warning") | foreground(TermColor::Yellow) | bold());
std::string banner = heading("System status", 40).str();
std::string separator = horizontal_line(40, '=').str();
std::string progress_bar = bar_text(20).str();
std::string labeled_bar = bar_text(20, "Loading", TermJustification::Center).str();
std::string section_line = msg_line("Network status", 40, TermJustification::Center);
```

Available effects include `foreground`, `background`, `bold`, `underline`,
`reverse_video`, and `line_graphics`. `horizontal_line`, `bar_text`, and
`heading` provide basic text shapes. Existing cursor and frame helpers remain
available for terminal layout.