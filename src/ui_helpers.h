#pragma once
#ifndef CATA_SRC_UI_HELPERS_H
#define CATA_SRC_UI_HELPERS_H

class ui_adaptor;

namespace catacurses
{
class window;
} // namespace catacurses

namespace ui_helpers
{

void full_screen_window( ui_adaptor &ui,
                         catacurses::window *w, catacurses::window *w_border = nullptr,
                         catacurses::window *w_header = nullptr, catacurses::window *w_footer = nullptr,
                         int *content_height = nullptr, int border_width = 1,
                         int header_height = 0, int footer_height = 0 );

} // namespace ui_helpers

#endif // CATA_SRC_UI_HELPERS_H
