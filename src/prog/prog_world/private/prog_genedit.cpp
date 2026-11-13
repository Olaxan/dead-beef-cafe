#include "prog_world.h"

#include "proc.h"
#include "world.h"
#include "host_utils.h"
#include "os.h"
#include "os_basic.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/app.hpp>             // for Component, App
#include <ftxui/component/captured_mouse.hpp>  // for ftxui
#include <ftxui/component/component.hpp>  // for Slider, Checkbox, Vertical, Renderer, Button, Input, Menu, Radiobox, Toggle
#include <ftxui/component/component_base.hpp>  // for ComponentBase
#include <ftxui/dom/elements.hpp>  // for separator, operator|, Element, size, xflex, text, WIDTH, hbox, vbox, EQUAL, border, GREATER_THAN

#include "CLI/CLI.hpp"
#include "ftxui_host.h"

#include <string>
#include <cstdlib>
#include <print>
#include <iostream>
#include <fstream>

#include <iso646.h>

ProcessTask Programs::CmdGenEdit(Proc& proc, std::vector<std::string> args)
{

	FtxuiHost host{component, term_width, term_height};

	co_await host.run(&proc, step_size);

	co_return 0;
}
