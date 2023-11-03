#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "diary.h"
#include "input.h"
#include "options.h"
#include "output.h"
#include "popup.h"
#include "scores_ui.h"
#include "string_editor_window.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"
#include "wcwidth.h"

namespace
{
/**print list scrollable, printed std::vector<std::string> as list with scrollbar*/
void print_list_scrollable( catacurses::window *win, const std::vector<std::string> &list,
                            int *selection, int entries_per_page, int xoffset, int width,
                            bool active, bool border, const report_color_error color_error )
{
    if( *selection < 0 && !list.empty() ) {
        *selection = static_cast<int>( list.size() ) - 1;
    } else if( *selection >= static_cast<int>( list.size() ) ) {
        *selection = 0;
    }
    const int borderspace = border ? 1 : 0;
    entries_per_page = entries_per_page - borderspace * 2;

    const int top_of_page = entries_per_page * ( *selection / entries_per_page );

    const int bottom_of_page = std::min<int>( top_of_page + entries_per_page, list.size() );

    const int line_width = width - 1 - borderspace;

    for( int i = top_of_page; i < bottom_of_page; i++ ) {

        const nc_color col = c_white;
        const int y = i - top_of_page;
        const bool highlight = *selection == i && active;
        const nc_color line_color = highlight ? hilite( col ) : col;
        std::string line_str = list[i];
        if( highlight ) {
            line_str = left_justify( remove_color_tags( line_str ), line_width );
        }
        trim_and_print( *win, point( xoffset + 1, y + borderspace ), line_width,
                        line_color, line_str, color_error );
    }
    if( border ) {
        draw_border( *win );
    }
    if( active && entries_per_page < static_cast<int>( list.size() ) ) {
        draw_scrollbar( *win, *selection, entries_per_page, list.size(), point( xoffset,
                        0 + borderspace ) );
    }

}

void print_list_scrollable( catacurses::window *win, const std::vector<std::string> &list,
                            int *selection, bool active, bool border,
                            const report_color_error color_error )
{
    print_list_scrollable( win, list, selection, getmaxy( *win ), 0, getmaxx( *win ),
                           active, border,
                           color_error );
}

std::vector<std::string>
text_to_list_scrollable( catacurses::window &win, const std::string &text, const bool border )
{
    const int width = getmaxx( win );
    const int borderspace = border ? 1 : 0;
    // -1 on the left for scroll bar and another -1 on the right reserved for cursor
    return foldstring( text, width - 2 - borderspace * 2 );
}

void print_list_scrollable( catacurses::window *win, const std::string &text, int *selection,
                            bool active,
                            bool border, const report_color_error color_error )
{
    std::vector<std::string> list = text_to_list_scrollable( *win, text, border );
    print_list_scrollable( win, list, selection, active, border, color_error );
}

// Print multiple strings, stacked vertically downwards from the specified point
template<typename ...Ts>
void mvwprintwa( const catacurses::window &win, point p, Ts... args )
{
    for( auto string : {
             args...
         } ) {
        mvwprintw( win, p, string );
        p += point_south;
    }
}

void draw_diary_border( catacurses::window *win )
{
    const point max( getmaxx( *win ) - 1, getmaxy( *win ) - 1 );
    const point mid = max / 2;
    // left, right vertical lines
    for( int i = 0; i < 4; i++ ) {
        mvwvline( *win, point( 0     + i, 4 ), '|', max.y - 4 - 4 + 1 );
        mvwvline( *win, point( max.x - i, 4 ), '|', max.y - 4 - 4 + 1 );
    }
    // middle vertical line
    mvwvline( *win,     point( mid.x,     4 ), '|', max.y - 4 - 4 + 1 );
    // top horizontal line
    mvwhline( *win,     point( 4,         0 ), '_', max.x - 4 - 4 + 1 );
    // bottom horizontal lines
    mvwhline( *win,     point( 4, max.y - 2 ), '_', max.x - 4 - 4 + 1 );
    mvwhline( *win,     point( 4, max.y - 1 ), '=', max.x - 4 - 4 + 1 );
    mvwhline( *win,     point( 4, max.y - 0 ), '-', max.x - 4 - 4 + 1 );

    wattron( *win, BORDER_COLOR );
    //top left corner
    mvwprintwa( *win, point_zero,
                "    ",
                ".-/|",
                "||||",
                "||||" );
    // bottom left corner
    mvwprintwa( *win, point( 0, max.y - 3 ),
                "||||",
                "||||",
                "||/=",
                "`'--" );
    // top right corner
    mvwprintwa( *win, point( max.x - 3, 0 ),
                "    ",
                "|\\-.",
                "||||",
                "||||" );
    // bottom right corner
    mvwprintwa( *win, max + point( -3, -3 ),
                "||||",
                "||||",
                "=\\||",
                "--''" );
    // top middle
    mvwprintwa( *win, point( mid.x - 1, 0 ),
                "   ",
                "\\ /",
                " | ",
                " | " );
    // bottom middle
    mvwprintwa( *win, point( mid.x - 1, max.y - 3 ),
                " | ",
                " | ",
                "\\|/",
                "___" );

    wattroff( *win, BORDER_COLOR );
}
} // namespace

static std::pair<point, point> diary_window_position()
{
    return {
        point( TERMX / 4, TERMY / 4 ),
        point( TERMX / 2, TERMY / 2 )
    };
}

void diary::show_diary_ui( diary *c_diary )
{
    catacurses::window w_tabs;
    catacurses::window w_diary;
    catacurses::window w_pages;
    catacurses::window w_pages_title;
    catacurses::window w_rightpage;
    catacurses::window w_leftpage;
    catacurses::window w_border;
    catacurses::window w_desc;
    catacurses::window w_head;
    catacurses::window w_info;
    enum class window_mode : int {PAGE_WIN = 0, CHANGE_WIN, TEXT_WIN, NUM_WIN, FIRST_WIN = 0, LAST_WIN = NUM_WIN - 1};
    window_mode currwin = window_mode::PAGE_WIN;

    const int diary_last_page = c_diary->pages.empty() ? 0 : ( c_diary->pages.size() - 1 );
    std::map<window_mode, int> selected = { {window_mode::PAGE_WIN, diary_last_page}, {window_mode::CHANGE_WIN, 0}, {window_mode::TEXT_WIN, 0} };

    enum class tab_mode : int {
        TAB_DIARY = 0,
        TAB_CREATURES,
        TAB_LORE,
        TAB_ACHIEVEMENTS,
        NUM_TABS,
        FIRST_TAB = 0,
        LAST_TAB = NUM_TABS - 1
    };
    tab_mode tab = tab_mode::FIRST_TAB;
    size_t tab_selection = 0;

    input_context ctxt( "DIARY" );
    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "NEW_PAGE" );
    ctxt.register_action( "DELETE PAGE" );
    ctxt.register_action( "EXPORT_DIARY" );
    ctxt.register_action( "VIEW_SCORES" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    ui_adaptor ui_tabs;
    ui_tabs.on_screen_resize( [&]( ui_adaptor & ui ) {
        const std::pair<point, point> beg_and_max = diary_window_position();
        const point &beg = beg_and_max.first;
        const point &max = beg_and_max.second;

        w_tabs = catacurses::newwin( max.y + 12, max.x * 3 / 10 + max.x + 10,
                                     point( beg.x - 5 - max.x * 3 / 10, beg.y - 9 ) );
        ui.position_from_window( w_tabs );
    } );
    ui_tabs.mark_resize();
    ui_tabs.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_tabs );

        const std::vector<std::pair<tab_mode, std::string>> tabs = {
            { tab_mode::TAB_DIARY, _( "DIARY" ) },
            { tab_mode::TAB_CREATURES, _( "CREATURES" ) },
            { tab_mode::TAB_LORE, _( "LORE" ) },
            { tab_mode::TAB_ACHIEVEMENTS, _( "ACHIEVEMENTS" ) },
        };
        draw_tabs( w_tabs, tabs, tab );
        draw_border_below_tabs( w_tabs );

        // border between pages list and book
        /*
        int border_x = diary_window_position().second.x * 3 / 10 + 1;
        for (int y = 3; y < getmaxy(w_tabs) - 1; y++) {
            mvwputch(w_tabs, point(border_x, y), BORDER_COLOR, LINE_XOXO);
        }
        mvwputch(w_tabs, point(border_x, 2), BORDER_COLOR, LINE_OXXX); // ^|^ - TODO: depending on tab placement, this should sometimes be + or empty
        mvwputch(w_tabs, point(border_x, getmaxy(w_tabs) - 1), BORDER_COLOR, LINE_XXOX); // _|_
        */

        wnoutrefresh( w_tabs );
    } );


    ui_adaptor ui_diary;
    ui_diary.on_screen_resize( [&]( ui_adaptor & ui ) {
        const std::pair<point, point> beg_and_max = diary_window_position();
        const point &beg = beg_and_max.first;
        const point &max = beg_and_max.second;
        const int midx = max.x / 2;

        w_leftpage = catacurses::newwin( max.y - 3, midx - 2, beg + point( 1, 2 ) );
        w_rightpage = catacurses::newwin( max.y - 3, max.x - midx - 2, beg + point( 2 + midx, 2 ) );
        w_border = catacurses::newwin( max.y + 5, max.x + 7, beg + point( -3, -3 ) );
        w_head = catacurses::newwin( 1, max.x - 2, beg + point_south + point( 1, -1 ) );

        ui.position_from_window( w_border );
    } );
    ui_diary.mark_resize();
    ui_diary.on_redraw( [&]( const ui_adaptor & ) {

        werase( w_border );
        werase( w_leftpage );
        werase( w_rightpage );
        draw_diary_border( &w_border );
        wnoutrefresh( w_border );

        switch( tab ) {
            case tab_mode::TAB_DIARY: {
                werase( w_head );
                print_list_scrollable( &w_leftpage, c_diary->get_change_list(), &selected[window_mode::CHANGE_WIN],
                                       currwin == window_mode::CHANGE_WIN, false, report_color_error::yes );
                print_list_scrollable( &w_rightpage, c_diary->get_page_text(), &selected[window_mode::TEXT_WIN],
                                       currwin == window_mode::TEXT_WIN, false, report_color_error::no );
                trim_and_print( w_head, point_south_east, getmaxx( w_head ) - 2, c_white,
                                c_diary->get_head_text() );
                wnoutrefresh( w_head );
            }
            break;
            case tab_mode::TAB_CREATURES: {
                center_print( w_border, 4, c_light_gray, "creatures" );
            }
            break;
            case tab_mode::TAB_LORE: {
                center_print( w_border, 4, c_light_gray, "lore" );
            }
            break;
            case tab_mode::TAB_ACHIEVEMENTS: {
                center_print( w_border, 4, c_light_gray, "achievements" );
            }
            break;
        }

        wnoutrefresh( w_leftpage );
        wnoutrefresh( w_rightpage );

    } );

    ui_adaptor ui_pages;
    ui_pages.on_screen_resize( [&]( ui_adaptor & ui ) {
        const std::pair<point, point> beg_and_max = diary_window_position();
        const point &beg = beg_and_max.first;
        const point &max = beg_and_max.second;

        w_pages_title = catacurses::newwin( 2, max.x * 3 / 10, point( beg.x - 4 - max.x * 3 / 10,
                                            beg.y - 6 ) );
        w_pages = catacurses::newwin( max.y + 6, max.x * 3 / 10, point( beg.x - 4 - max.x * 3 / 10,
                                      beg.y - 4 ) );

        ui.position_from_window( w_pages );
    } );
    ui_pages.mark_resize();
    ui_pages.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_pages_title );
        werase( w_pages );
        print_list_scrollable( &w_pages, c_diary->get_pages_list(), &selected[window_mode::PAGE_WIN],
                               currwin == window_mode::PAGE_WIN, false, report_color_error::yes );
        size_t num_pages = c_diary->get_pages_list().size();
        std::string title = num_pages == 1 ? _( "1 page" ) : string_format( _( "%d pages" ), num_pages );
        center_print( w_pages_title, 1, c_light_gray, title );
        wnoutrefresh( w_pages_title );
        wnoutrefresh( w_pages );
    } );

    ui_adaptor ui_desc;
    ui_desc.on_screen_resize( [&]( ui_adaptor & ui ) {
        const std::pair<point, point> beg_and_max = diary_window_position();
        const point &beg = beg_and_max.first;
        const point &max = beg_and_max.second;

        w_desc = catacurses::newwin( 3, max.x + 7, beg + point( -3, - 6 ) );

        ui.position_from_window( w_desc );
    } );
    ui_desc.mark_resize();
    ui_desc.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_desc );
        center_print( w_desc, 0, c_light_gray, string_format( _( "%s´s Diary" ), c_diary->owner ) );
        std::string desc = string_format( _( "%s, %s, %s, %s" ),
                                          ctxt.get_desc( "NEW_PAGE", _( "New page" ), input_context::allow_all_keys ),
                                          ctxt.get_desc( "CONFIRM", _( "Edit text" ), input_context::allow_all_keys ),
                                          ctxt.get_desc( "DELETE PAGE", _( "Delete page" ), input_context::allow_all_keys ),
                                          ctxt.get_desc( "EXPORT_DIARY", _( "Export diary" ), input_context::allow_all_keys )
                                        );
        center_print( w_desc, 1,  c_white, desc );
        wnoutrefresh( w_desc );
    } );

    ui_adaptor ui_info;
    ui_info.on_screen_resize( [&]( ui_adaptor & ui ) {
        const std::pair<point, point> beg_and_max = diary_window_position();
        const point &beg = beg_and_max.first;
        const point &max = beg_and_max.second;

        w_info = catacurses::newwin( std::clamp( 3, max.y / 2 - 4, 7 ), max.x + 9, beg + point( -4,
                                     3 + max.y + ( max.y > 12 ) ) );

        ui.position_from_window( w_info );
    } );
    ui_info.mark_resize();
    ui_info.on_redraw( [&]( const ui_adaptor & ) {
        if( tab == tab_mode::TAB_DIARY ) {
            werase( w_info );
            draw_border( w_info );
            center_print( w_info, 0, c_light_gray, string_format( _( "Info" ) ) );
            if( currwin == window_mode::CHANGE_WIN || currwin == window_mode::TEXT_WIN ) {
                fold_and_print( w_info, point_south_east, getmaxx( w_info ) - 2, c_white,
                                c_diary->get_desc_map()[selected[window_mode::CHANGE_WIN]] );
            }
        }
        wnoutrefresh( w_info );
    } );

    while( true ) {

        if( ( !c_diary->pages.empty() &&
              selected[window_mode::PAGE_WIN] >= static_cast<int>( c_diary->pages.size() ) ) ||
            ( c_diary->pages.empty() && selected[window_mode::PAGE_WIN] != 0 ) ) {
            selected[window_mode::PAGE_WIN] = 0;
        }
        selected[window_mode::PAGE_WIN] = c_diary->set_opened_page( selected[window_mode::PAGE_WIN] );
        ui_tabs.invalidate_ui();
        ui_diary.invalidate_ui();
        ui_pages.invalidate_ui();
        ui_desc.invalidate_ui();
        ui_info.invalidate_ui();
        ui_manager::redraw_invalidated();
        const std::string action = ctxt.handle_input();
        if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            // change tab
            // necessary to use inc_clamp_wrap()
            static_assert( static_cast<int>( tab_mode::FIRST_TAB ) == 0 );
            tab = inc_clamp_wrap( tab, action == "NEXT_TAB", tab_mode::NUM_TABS );
            tab_selection = 0;
            ui_info.mark_resize(); // not visible on all tabs
            ui_diary.mark_resize(); // different layout on different tabs
        } else if( action == "LEFT" || action == "RIGHT" ) {
            // change diary window
            // necessary to use inc_clamp_wrap()
            static_assert( static_cast<int>( window_mode::FIRST_WIN ) == 0 );
            currwin = inc_clamp_wrap( currwin, action == "RIGHT" ||
                                      action == "NEXT_TAB", window_mode::NUM_WIN );
            selected[window_mode::TEXT_WIN] = 0;
        } else if( navigate_ui_list( action, selected[currwin], 10,
                                     currwin == window_mode::PAGE_WIN ? c_diary->pages.size()
                                     : currwin == window_mode::CHANGE_WIN ? c_diary->change_list.size()
                                     : text_to_list_scrollable( w_rightpage, c_diary->get_page_text(), false ).size(), true ) ) {
            // size in navigate_ui_list above is redundant with print_list_scrollable's wrapping effect during redraw
            if( currwin == window_mode::PAGE_WIN ) {
                selected[window_mode::CHANGE_WIN] = 0;
                selected[window_mode::TEXT_WIN] = 0;
            }
        } else if( action == "CONFIRM" ) {
            if( !c_diary->pages.empty() ) {
                c_diary->edit_page_ui( [&]() {
                    return w_rightpage;
                } );
            }
        } else if( action == "NEW_PAGE" ) {
            c_diary->new_page();
            selected[window_mode::PAGE_WIN] = c_diary->pages.size() - 1;
            currwin = window_mode::PAGE_WIN;
        } else if( action == "VIEW_SCORES" ) {
            show_scores_ui( g->achievements(), g->stats(), g->get_kill_tracker() );
        } else if( action == "DELETE PAGE" ) {
            if( !c_diary->pages.empty() ) {
                if( query_yn( _( "Really delete Page?" ) ) ) {
                    c_diary->delete_page();
                    if( selected[window_mode::PAGE_WIN] >= static_cast<int>( c_diary->pages.size() ) ) {
                        selected[window_mode::PAGE_WIN] --;
                    }
                }
            }
        } else if( action == "EXPORT_DIARY" ) {
            if( query_yn( _( "Export Diary as .txt?" ) ) ) {
                c_diary->export_to_txt();
            }

        } else if( action == "QUIT" ) {
            break;
        }

    }
}

void diary::edit_page_ui( const std::function<catacurses::window()> &create_window )
{
    // Modify the stored text so the new text is displayed after exiting from
    // the editor window and before confirming or canceling the y/n query.
    std::string &new_rightpage = get_page_ptr()->m_text;
    const std::string old_text = new_rightpage;

    string_editor_window ed( create_window, new_rightpage );

    do {
        const std::pair<bool, std::string> result = ed.query_string();
        new_rightpage = result.second;

        // Confirmed or unchanged
        if( result.first || old_text == new_rightpage ) {
            break;
        }

        const query_ynq_result res = query_ynq( _( "Save entry?" ) );
        if( res == query_ynq_result::yes ) {
            break;
        } else if( res == query_ynq_result::no ) {
            new_rightpage = old_text;
            break;
        }
    } while( true );
}
