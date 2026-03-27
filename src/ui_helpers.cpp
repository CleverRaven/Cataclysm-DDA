#include "ui_helpers.h"

#include "cursesdef.h"
#include "output.h"
#include "point.h"
#include "ui_manager.h"

void ui_helpers::full_screen_window( ui_adaptor &ui,
                                     catacurses::window *w, catacurses::window *const w_border,
                                     catacurses::window *const w_header, catacurses::window *const w_footer,
                                     int *const content_height, const int border_width,
                                     const int header_height, const int footer_height )
{
    const int content_width = FULL_SCREEN_WIDTH - ( w_border ? border_width * 2 : 0 );
    const int new_content_height =
        FULL_SCREEN_HEIGHT
        - ( w_border ? border_width * 2 : 0 )
        - header_height
        - footer_height;
    if( content_height ) {
        *content_height = new_content_height;
    }
    point offset( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                  TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 );
    if( w_border ) {
        *w_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, offset );
        offset += point::south_east * border_width;
    }
    if( w_header ) {
        *w_header = catacurses::newwin( header_height, content_width, offset );
    }
    offset += point::south * header_height;
    *w = catacurses::newwin( new_content_height, content_width, offset );
    offset += point::south * new_content_height;
    if( w_footer ) {
        *w_footer = catacurses::newwin( footer_height, content_width, offset );
    }

    ui.position_from_window( w_border ? *w_border : *w );
}
