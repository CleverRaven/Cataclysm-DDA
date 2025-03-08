#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "diary.h"
#include "input_context.h"
#include "output.h"
#include "point.h"
#include "scores_ui.h"
#include "string_editor_window.h"
#include "string_formatter.h"
#include "translations.h"
#include "uilist.h"
#include "ui_manager.h"

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
        p += point::south;
    }
}

void draw_diary_border( catacurses::window &win )
{
    const point max( getmaxx( win ) - 1, getmaxy( win ) - 1 );
    const point mid = max / 2;

    wattron( win, BORDER_COLOR );

    // left, right vertical lines
    for( int i = 0; i < 4; i++ ) {
        mvwvline( win, point( 0     + i, 4 ), '|', max.y - 4 - 4 + 1 );
        mvwvline( win, point( max.x - i, 4 ), '|', max.y - 4 - 4 + 1 );
    }
    // middle vertical line
    mvwvline( win,     point( mid.x,     4 ), '|', max.y - 4 - 4 + 1 );
    // top horizontal line
    mvwhline( win,     point( 4,         0 ), '_', max.x - 4 - 4 + 1 );
    // bottom horizontal lines
    mvwhline( win,     point( 4, max.y - 2 ), '_', max.x - 4 - 4 + 1 );
    mvwhline( win,     point( 4, max.y - 1 ), '=', max.x - 4 - 4 + 1 );
    mvwhline( win,     point( 4, max.y - 0 ), '-', max.x - 4 - 4 + 1 );

    //top left corner
    mvwprintwa( win, point::zero,
                "    ",
                ".-/|",
                "||||",
                "||||" );
    // bottom left corner
    mvwprintwa( win, point( 0, max.y - 3 ),
                "||||",
                "||||",
                "||/=",
                "`'--" );
    // top right corner
    mvwprintwa( win, point( max.x - 3, 0 ),
                "    ",
                "|\\-.",
                "||||",
                "||||" );
    // bottom right corner
    mvwprintwa( win, max + point( -3, -3 ),
                "||||",
                "||||",
                "=\\||",
                "--''" );
    // top middle
    mvwprintwa( win, point( mid.x - 1, 0 ),
                "   ",
                "\\ /",
                " | ",
                " | " );
    // bottom middle
    mvwprintwa( win, point( mid.x - 1, max.y - 3 ),
                " | ",
                " | ",
                "\\|/",
                "___" );

    wattroff( win, BORDER_COLOR );
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
    catacurses::window w_diary;
    catacurses::window w_pages;
    catacurses::window w_text;
    catacurses::window w_changes;
    catacurses::window w_border;
    catacurses::window w_desc;
    catacurses::window w_head;
    catacurses::window w_info;
    enum class window_mode : int {PAGE_WIN = 0, CHANGE_WIN, TEXT_WIN, NUM_WIN, FIRST_WIN = 0, LAST_WIN = NUM_WIN - 1};
    window_mode currwin = window_mode::PAGE_WIN;

    const int diary_last_page = c_diary->pages.empty() ? 0 : ( c_diary->pages.size() - 1 );
    std::map<window_mode, int> selected = { {window_mode::PAGE_WIN, diary_last_page}, {window_mode::CHANGE_WIN, 0}, {window_mode::TEXT_WIN, 0} };

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

    ui_adaptor ui_diary;
    ui_diary.on_screen_resize( [&]( ui_adaptor & ui ) {
        const std::pair<point, point> beg_and_max = diary_window_position();
        const point &beg = beg_and_max.first;
        const point &max = beg_and_max.second;
        const int midx = max.x / 2;

        w_changes = catacurses::newwin( max.y - 3, midx - 1, beg + point( 0, 3 ) );
        w_text = catacurses::newwin( max.y - 3, max.x - midx - 1, beg + point( 2 + midx, 3 ) );
        w_border = catacurses::newwin( max.y + 5, max.x + 9, beg + point( -4, -2 ) );
        w_head = catacurses::newwin( 1, max.x, beg + point::south );

        ui.position_from_window( w_border );
    } );
    ui_diary.mark_resize();
    ui_diary.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_changes );
        werase( w_text );
        werase( w_border );
        werase( w_head );

        draw_diary_border( w_border );

        print_list_scrollable( &w_changes, c_diary->get_change_list(), &selected[window_mode::CHANGE_WIN],
                               currwin == window_mode::CHANGE_WIN, false, report_color_error::yes );
        print_list_scrollable( &w_text, c_diary->get_page_text(), &selected[window_mode::TEXT_WIN],
                               currwin == window_mode::TEXT_WIN, false, report_color_error::no );

        trim_and_print( w_head, point::south_east, getmaxx( w_head ) - 2, c_white,
                        c_diary->get_head_text() );

        wnoutrefresh( w_border );
        wnoutrefresh( w_head );
        wnoutrefresh( w_changes );
        wnoutrefresh( w_text );
    } );

    ui_adaptor ui_pages;
    ui_pages.on_screen_resize( [&]( ui_adaptor & ui ) {
        const std::pair<point, point> beg_and_max = diary_window_position();
        const point &beg = beg_and_max.first;
        const point &max = beg_and_max.second;

        w_pages = catacurses::newwin( max.y + 5, max.x * 3 / 10 + 1, point( beg.x - 5 - max.x * 3 / 10,
                                      beg.y - 2 ) );

        ui.position_from_window( w_pages );
    } );
    ui_pages.mark_resize();
    ui_pages.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_pages );

        print_list_scrollable( &w_pages, c_diary->get_pages_list(), &selected[window_mode::PAGE_WIN],
                               currwin == window_mode::PAGE_WIN, true, report_color_error::yes );
        center_print( w_pages, 0, c_light_gray, string_format( _( "pages: %d" ),
                      c_diary->get_pages_list().size() ) );

        wnoutrefresh( w_pages );
    } );

    ui_adaptor ui_desc;
    ui_desc.on_screen_resize( [&]( ui_adaptor & ui ) {
        const std::pair<point, point> beg_and_max = diary_window_position();
        const point &beg = beg_and_max.first;
        const point &max = beg_and_max.second;

        w_desc = catacurses::newwin( 4, max.x * 3 / 10 + max.x + 10, point( beg.x - 5 - max.x * 3 / 10,
                                     beg.y - 6 ) );

        ui.position_from_window( w_desc );
    } );
    ui_desc.mark_resize();
    ui_desc.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_desc );

        draw_border( w_desc );
        center_print( w_desc, 0, c_light_gray, string_format( _( "%s's Diary" ), c_diary->owner ) );
        std::string desc = string_format( _( "%s, %s, %s, %s" ),
                                          ctxt.get_desc( "NEW_PAGE", _( "New page" ), input_context::allow_all_keys ),
                                          ctxt.get_desc( "CONFIRM", _( "Edit text" ), input_context::allow_all_keys ),
                                          ctxt.get_desc( "DELETE PAGE", _( "Delete page" ), input_context::allow_all_keys ),
                                          ctxt.get_desc( "EXPORT_DIARY", _( "Export diary" ), input_context::allow_all_keys )
                                        );
        center_print( w_desc, 1,  c_white, desc );
        center_print( w_desc, 2,  c_white, ctxt.get_desc( "VIEW_SCORES",
                      _( "View achievements, scores, and kills" ), input_context::allow_all_keys ) );

        wnoutrefresh( w_desc );
    } );

    ui_adaptor ui_info;
    ui_info.on_screen_resize( [&]( ui_adaptor & ui ) {
        const std::pair<point, point> beg_and_max = diary_window_position();
        const point &beg = beg_and_max.first;
        const point &max = beg_and_max.second;

        int lines = std::clamp( max.y / 2 - 4, 3, 7 );

        w_info = catacurses::newwin( lines, max.x + 9, beg + point( -4,
                                     3 + max.y + ( max.y > 12 ) ) );

        ui.position_from_window( w_info );
    } );
    ui_info.mark_resize();
    ui_info.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_info );

        draw_border( w_info );
        center_print( w_info, 0, c_light_gray, string_format( _( "Info" ) ) );
        if( currwin == window_mode::CHANGE_WIN || currwin == window_mode::TEXT_WIN ) {
            fold_and_print( w_info, point::south_east, getmaxx( w_info ) - 2, c_white,
                            c_diary->get_desc_map()[selected[window_mode::CHANGE_WIN]] );
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
        ui_diary.invalidate_ui();
        ui_pages.invalidate_ui();
        ui_desc.invalidate_ui();
        ui_info.invalidate_ui();
        ui_manager::redraw_invalidated();
        const std::string action = ctxt.handle_input();
        if( action == "LEFT" || action == "PREV_TAB" || action == "RIGHT" || action == "NEXT_TAB" ) {
            // necessary to use inc_clamp_wrap()
            static_assert( static_cast<int>( window_mode::FIRST_WIN ) == 0 );
            currwin = inc_clamp_wrap( currwin, action == "RIGHT" || action == "NEXT_TAB",
                                      window_mode::NUM_WIN );
            selected[window_mode::TEXT_WIN] = 0;
        } else if( navigate_ui_list( action, selected[currwin], 10,
                                     currwin == window_mode::PAGE_WIN ? c_diary->pages.size()
                                     : currwin == window_mode::CHANGE_WIN ? c_diary->change_list.size()
                                     : text_to_list_scrollable( w_text, c_diary->get_page_text(), false ).size(), true ) ) {
            // size in navigate_ui_list above is redundant with print_list_scrollable's wrapping effect during redraw
            if( currwin == window_mode::PAGE_WIN ) {
                selected[window_mode::CHANGE_WIN] = 0;
                selected[window_mode::TEXT_WIN] = 0;
            }
        } else if( action == "CONFIRM" ) {
            if( !c_diary->pages.empty() ) {
                c_diary->edit_page_ui( [&]() {
                    return w_text;
                } );
            }
        } else if( action == "NEW_PAGE" ) {
            c_diary->new_page();
            selected[window_mode::PAGE_WIN] = c_diary->pages.size() - 1;
            currwin = window_mode::PAGE_WIN;
        } else if( action == "VIEW_SCORES" ) {
            show_scores_ui();
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
    std::string &new_text = get_page_ptr()->m_text;
    const std::string old_text = new_text;

    string_editor_window ed( create_window, new_text );

    do {
        const std::pair<bool, std::string> result = ed.query_string();
        new_text = result.second;

        // Confirmed or unchanged
        if( result.first || old_text == new_text ) {
            break;
        }

        const query_ynq_result res = query_ynq( _( "Save entry?" ) );
        if( res == query_ynq_result::yes ) {
            break;
        } else if( res == query_ynq_result::no ) {
            new_text = old_text;
            break;
        }
    } while( true );
}
