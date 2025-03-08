#include "smart_controller_ui.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "cata_utility.h"
#include "color.h"
#include "cursesdef.h"
#include "output.h"
#include "point.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "uilist.h"
#include "ui_manager.h"

static catacurses::window init_window()
{
    const int width = smart_controller_ui::WIDTH;
    const int height = smart_controller_ui::HEIGHT;
    const point p( std::max( 0, ( TERMX - width ) / 2 ), std::max( 0, ( TERMY - height ) / 2 ) );
    return catacurses::newwin( height, width, p );
}

smart_controller_settings::smart_controller_settings( bool &enabled, int &battery_lo,
        int &battery_hi ) : enabled( enabled ), battery_lo( battery_lo ), battery_hi( battery_hi ) {}

smart_controller_ui::smart_controller_ui( smart_controller_settings initial_settings ) :
    win( init_window() ), ctxt( "SMART_ENGINE_CONTROLLER" ), settings( initial_settings )
{
    ctxt.register_directions();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "NEXT_TAB" );
}

void smart_controller_ui::refresh()
{
    werase( win );
    draw_border( win );

    const nc_color gray = c_light_gray;
    const nc_color white = c_white;
    const nc_color lgreen = c_light_green;
    const nc_color red = c_red;

    // header
    const std::string title =  _( "Smart Engine Controller ® Interface" );
    mvwprintz( win, point( ( WIDTH - title.length() ) / 2, 1 ), white, title );
    mvwhline( win, point( 1, 2 ), BORDER_COLOR, LINE_OXOX, WIDTH - 2 );

    // for menu items, y points to the center of the menu item vertical space
    int y = 3 + MENU_ITEM_HEIGHT / 2;

    // enabled flag
    mvwprintz( win, point( LEFT_MARGIN, y ), gray,  "[ ]" );
    if( settings.enabled ) {
        mvwprintz( win, point( LEFT_MARGIN + 1, y ), white, "X" );
    }
    mvwprintz( win, point( LEFT_MARGIN + 4, y ),
               selection == smart_controller_ui_selection::enabled ? hilite( white ) : gray,
               _( "Enabled" ) );

    // battery % slider
    y += MENU_ITEM_HEIGHT;
    mvwprintz( win, point( LEFT_MARGIN, y - 1 ),
               selection == smart_controller_ui_selection::lo_and_hi_slider ? white : gray,
               _( "Electric motor use (%% battery)" ) );

    int lo_slider_x = settings.battery_lo * SLIDER_W / 100;
    int hi_slider_x = settings.battery_hi * SLIDER_W / 100 ;
    // print selected % numbers
    std::string battery_low_text = string_format( "%d%%", settings.battery_lo );
    mvwprintz( win, point( LEFT_MARGIN + lo_slider_x - battery_low_text.length() + 1, y + 2 ),
               selection == smart_controller_ui_selection::lo_and_hi_slider &&
               slider == 0 ? hilite( white ) : red, battery_low_text );
    mvwprintz( win, point( LEFT_MARGIN + hi_slider_x, y + 2 ),
               selection == smart_controller_ui_selection::lo_and_hi_slider &&
               slider == 1 ? hilite( white ) : lgreen, "%d%%", settings.battery_hi );
    // draw slider horizontal line
    for( int i = 0; i < SLIDER_W; ++i ) {
        nc_color col = selection == smart_controller_ui_selection::lo_and_hi_slider ? white : gray;
        mvwprintz( win, point( LEFT_MARGIN + i, y + 1 ), col, "-" );
    }
    // print LO and HI on the slider line
    //~ abbreviation for the "LOW" battery zone displayed on the Smart Controller UI slider. Keep it short (2-3 chars)
    std::string lo_text = _( "LO" );
    mvwprintz( win, point( LEFT_MARGIN + ( lo_slider_x - lo_text.length() ) / 2, y + 1 ), red,
               lo_text );
    //~ abbreviation for the "HIGH" battery zone displayed on the Smart Controller UI slider. Keep it short (2-3 chars)
    std::string hi_text = _( "HI" );
    int hi_text_l = hi_text.length();
    mvwprintz( win, point( LEFT_MARGIN + hi_slider_x + std::max( ( SLIDER_W - hi_slider_x -
                           hi_text_l ) / 2, 1 ), y + 1 ), lgreen, hi_text );
    // print bars on the slider
    mvwprintz( win, point( LEFT_MARGIN + lo_slider_x, y + 1 ),
               selection == smart_controller_ui_selection::lo_and_hi_slider &&
               slider == 0 ? hilite( white ) : red, "|" );
    mvwprintz( win, point( LEFT_MARGIN + hi_slider_x, y + 1 ),
               selection == smart_controller_ui_selection::lo_and_hi_slider &&
               slider == 1 ? hilite( white ) : lgreen, "|" );

    // user manual
    y += MENU_ITEM_HEIGHT;
    mvwprintz( win, point( LEFT_MARGIN, y ),
               selection == smart_controller_ui_selection::manual ? hilite( white ) : gray,
               _( "User manual" ) );

    // key descriptions
    std::string keys_text = string_format(
                                _( "Use [<color_yellow>%s</color> and <color_yellow>%s</color>] to select option.\n"
                                   "Use [<color_yellow>%s</color>] to change value.\n"
                                   "Use [<color_yellow>%s</color> or <color_yellow>%s</color>] to switch between sliders.\n"
                                   "Use [<color_yellow>%s</color> and <color_yellow>%s</color>] to move sliders."
                                   "Use [<color_yellow>%s</color>] to apply changes and quit." ),
                                ctxt.get_desc( "UP" ),
                                ctxt.get_desc( "DOWN" ),
                                ctxt.get_desc( "CONFIRM" ),
                                ctxt.get_desc( "NEXT_TAB" ),
                                ctxt.get_desc( "CONFIRM" ),
                                ctxt.get_desc( "LEFT" ),
                                ctxt.get_desc( "RIGHT" ),
                                ctxt.get_desc( "QUIT" ) );

    int keys_text_w =  WIDTH - 2;
    int keys_text_lines_n = foldstring( keys_text, keys_text_w ).size();
    fold_and_print( win, point( 1, HEIGHT - 1 - keys_text_lines_n ), keys_text_w, gray, keys_text );

    wnoutrefresh( win );
}

void smart_controller_ui::control()
{
    ui_adaptor ui;
    ui.on_screen_resize( [this]( ui_adaptor & ui ) {
        win = init_window();
        ui.position_from_window( win );
    } );
    ui.mark_resize();
    ui.on_redraw( [this]( const ui_adaptor & ) {
        refresh();
    } );

    std::string action;

    do {
        ui_manager::redraw();
        action = ctxt.handle_input();

        if( action == "CONFIRM" || ( action == "NEXT_TAB" &&
                                     selection == smart_controller_ui_selection::lo_and_hi_slider ) ) {
            switch( selection ) {
                case smart_controller_ui_selection::enabled:
                    settings.enabled = !settings.enabled;
                    break;
                case smart_controller_ui_selection::lo_and_hi_slider:
                    slider = 1 - slider;
                    break;
                case smart_controller_ui_selection::manual:
                    const translation manual =
                        SNIPPET.random_from_category( "smart_engine_controller_manual" ).value_or( translation() );

                    scrollable_text( [&] {
                        //note: ui.on_screen_resize will be called first and reinit `win`
                        return win;
                    }, _( "Smart Engine Controller ® User Manual" ), manual.translated() );

                    break;
            }
        } else if( action == "UP" || action == "DOWN" ) {
            selection = inc_clamp_wrap( selection, action == "DOWN", MENU_ITEMS_N );
        } else if( selection == smart_controller_ui_selection::lo_and_hi_slider && ( action == "LEFT" ||
                   action == "RIGHT" ) ) {
            const int dx = action == "RIGHT" ? 1 : -1;

            if( slider == 0 ) {
                settings.battery_lo = clamp( ( settings.battery_lo / 5 + dx ) * 5, 5, 90 );
                // keep the min space of 5 between this and other slider
                settings.battery_hi = std::max( settings.battery_lo + 5, settings.battery_hi );
            } else {
                settings.battery_hi = clamp( ( settings.battery_hi / 5 + dx ) * 5, 10, 95 );
                // keep the min space of 5 between this and other slider
                settings.battery_lo = std::min( settings.battery_hi - 5, settings.battery_lo );
            }
        }

    } while( action != "QUIT" );
}
