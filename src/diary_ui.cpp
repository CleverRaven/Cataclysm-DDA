#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <map>
#include <string>
#include <vector>


#include "color.h"
#include "debug.h"
#include "input.h"
#include "output.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"
#include "cursesdef.h"
#include "wcwidth.h"
#include "string_editor_window.h"
#include "diary.h"


namespace
{
/**print list scrollable, printed std::vector<std::string> as list with scrollbar*/
void print_list_scrollable( catacurses::window *win, std::vector<std::string> list, int *selection,
                            int entries_per_page, int xoffset, int width, bool active, bool border )
{
    if( *selection < 0 && !list.empty() ) {
        *selection = static_cast<int>( list.size() ) - 1;
    } else if( *selection >= static_cast<int>( list.size() ) ) {
        *selection = 0;
    }
    const int borderspace = border ? 1 : 0;
    entries_per_page = entries_per_page - borderspace * 2;


    const int top_of_page = entries_per_page * ( *selection / entries_per_page );

    const int bottom_of_page =
        std::min( top_of_page + entries_per_page, static_cast<int>( list.size() ) );

    for( int i = top_of_page; i < bottom_of_page; i++ ) {

        const nc_color col = c_white;
        const int y = i - top_of_page;
        trim_and_print( *win, point( xoffset + 1, y + borderspace ), width - 1 - borderspace,
                        ( static_cast<int>( *selection ) == i && active ) ? hilite( col ) : col,
                        ( static_cast<int>( *selection ) == i && active ) ? remove_color_tags( list[i] ) : list[i] );
    }
    if( border ) {
        draw_border( *win );
    }
    if( active && entries_per_page < static_cast<int>( list.size() ) ) {
        draw_scrollbar( *win, *selection, entries_per_page, list.size(), point( xoffset,
                        0 + borderspace ) );
    }

}

void print_list_scrollable( catacurses::window *win, std::vector<std::string> list, int *selection,
                            bool active, bool border )
{
    print_list_scrollable( win, list, selection, getmaxy( *win ), 0, getmaxx( *win ), active, border );
}

void print_list_scrollable( catacurses::window *win, std::string text, int *selection,
                            int entries_per_page, int xoffset, int width, bool active, bool border )
{
    int borderspace = border ? 1 : 0;
    std::vector<std::string> list = foldstring( text, width - 1 - borderspace * 2 );
    print_list_scrollable( win, list, selection, entries_per_page, xoffset, width, active, border );
}

void print_list_scrollable( catacurses::window *win, std::string text, int *selection, bool active,
                            bool border )
{
    print_list_scrollable( win, text, selection, getmaxy( *win ), 0, getmaxx( *win ), active, border );
}

void draw_diary_border( catacurses::window *win, const nc_color &color = c_white )
{
    wattron( *win, color );

    const point max( getmaxx( *win ) - 1, getmaxy( *win ) - 1 );
    const int midx = max.x / 2;
    for( int i = 4; i <= max.y - 4; i++ ) {
        mvwprintw( *win, point( 0, i ), "||||" );
        mvwprintw( *win, point( max.x - 3, i ), "||||" );
        mvwprintw( *win, point( midx, i ), " | " );
    }
    for( int i = 4; i <= max.x - 4; i++ ) {
        if( !( i >= midx && i <= midx + 2 ) ) {
            mvwprintw( *win, point( i, 0 ), "____" );
            mvwprintw( *win, point( i, max.y - 2 ), "____" );
            mvwprintw( *win, point( i, max.y - 1 ), "====" );
            mvwprintw( *win, point( i, max.y ), "----" );
        }
    }
    //top left corner
    mvwprintw( *win, point_zero, "    " );
    mvwprintw( *win, point_south, ".-/|" );
    mvwprintw( *win, point( 0, 2 ), "||||" );
    mvwprintw( *win, point( 0, 3 ), "||||" );
    //bottom left corner
    mvwprintw( *win, point( 0, max.y - 3 ), "||||" );
    mvwprintw( *win, point( 0, max.y - 2 ), "||||" );
    mvwprintw( *win, point( 0, max.y - 1 ), "||/=" );
    mvwprintw( *win, point( 0, max.y - 0 ), "`'--" );
    //top right corner
    mvwprintw( *win, point( max.x - 3, 0 ), "    " );
    mvwprintw( *win, point( max.x - 3, 1 ), "|\\-." );
    mvwprintw( *win, point( max.x - 3, 2 ), "||||" );
    mvwprintw( *win, point( max.x - 3, 3 ), "||||" );
    //bottom right corner
    mvwprintw( *win, point( max.x - 3, max.y - 3 ), "||||" );
    mvwprintw( *win, point( max.x - 3, max.y - 2 ), "||||" );
    mvwprintw( *win, point( max.x - 3, max.y - 1 ), "=\\||" );
    mvwprintw( *win, point( max.x - 3, max.y - 0 ), "--''" );
    //mid top
    mvwprintw( *win, point( midx, 0 ), "   " );
    mvwprintw( *win, point( midx, 1 ), "\\ /" );
    mvwprintw( *win, point( midx, 2 ), " | " );
    mvwprintw( *win, point( midx, 3 ), " | " );
    //mid bottom
    mvwprintw( *win, point( midx, max.y - 3 ), " | " );
    mvwprintw( *win, point( midx, max.y - 2 ), " | " );
    mvwprintw( *win, point( midx, max.y - 1 ), "\\|/" );
    mvwprintw( *win, point( midx, max.y - 0 ), "___" );
    wattroff( *win, color );
}
} // namespace

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


    std::map<window_mode, int> selected = { {window_mode::PAGE_WIN, 0}, {window_mode::CHANGE_WIN, 0}, {window_mode::TEXT_WIN, 0} };


    input_context ctxt( "DIARY" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "NEW_PAGE" );
    ctxt.register_action( "DELETE PAGE" );
    ctxt.register_action( "EXPORT_Diary" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        w_diary = new_centered_win( TERMY / 2, TERMX / 2 );
        const point max( getmaxx( w_diary ), getmaxy( w_diary ) );
        const point beg( getbegx( w_diary ), getbegy( w_diary ) );
        int midx = max.x / 2;

        w_pages = catacurses::newwin( max.y + 5, max.x * 3 / 10, point( beg.x - 5 - max.x * 3 / 10,
                                      beg.y - 2 ) );
        w_changes = catacurses::newwin( max.y - 3, midx - 1, point( beg.x, beg.y + 3 ) );
        w_text = catacurses::newwin( max.y - 3, midx - 2, point( beg.x + midx + 2, beg.y + 3 ) );
        w_border = catacurses::newwin( max.y + 5, max.x + 9, point( beg.x - 4, beg.y - 2 ) );
        w_desc = catacurses::newwin( 3, max.x * 3 / 10 + max.x + 10, point( beg.x - 5 - max.x * 3 / 10,
                                     beg.y - 6 ) );
        w_head = catacurses::newwin( 1, max.x, point( beg.x, beg.y + 1 ) );
        w_info = catacurses::newwin( ( max.y / 2 - 4 > 7 ) ? 7 : max.y / 2 - 4, max.x + 9, point( beg.x - 4,
                                     beg.y + max.y + 4 ) );


        ui.position_from_window( w_diary );
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_diary );
        werase( w_pages );
        werase( w_changes );
        werase( w_text );
        werase( w_border );
        werase( w_desc );
        werase( w_head );
        werase( w_info );


        draw_border( w_diary );
        draw_border( w_desc );
        draw_border( w_info );
        draw_diary_border( &w_border );


        center_print( w_desc, 0, c_light_gray, string_format( _( "%s´s Diary" ), c_diary->owner ) );
        center_print( w_info, 0, c_light_gray, string_format( _( "Info" ) ) );

        std::string desc = string_format( _( "%s, %s, %s, %s" ),
                                          ctxt.get_desc( "NEW_PAGE", "new page", input_context::allow_all_keys ),
                                          ctxt.get_desc( "CONFIRM", "Edit text", input_context::allow_all_keys ),
                                          ctxt.get_desc( "DELETE PAGE", "Delete page", input_context::allow_all_keys ),
                                          ctxt.get_desc( "EXPORT_Diary", "Export diary", input_context::allow_all_keys )
                                        );
        center_print( w_desc, 1,  c_white, desc );

        selected[window_mode::PAGE_WIN] = c_diary->set_opend_page( selected[window_mode::PAGE_WIN] );
        print_list_scrollable( &w_pages, c_diary->get_pages_list(), &selected[window_mode::PAGE_WIN],
                               currwin == window_mode::PAGE_WIN, true );
        print_list_scrollable( &w_changes, c_diary->get_change_list(), &selected[window_mode::CHANGE_WIN],
                               currwin == window_mode::CHANGE_WIN, false );
        print_list_scrollable( &w_text, c_diary->get_page_text(), &selected[window_mode::TEXT_WIN],
                               currwin == window_mode::TEXT_WIN, false );
        trim_and_print( w_head, point_south_east, getmaxx( w_head ) - 2, c_white,
                        c_diary->get_head_text() );
        if( currwin == window_mode::CHANGE_WIN || currwin == window_mode::TEXT_WIN ) {
            fold_and_print( w_info, point_south_east, getmaxx( w_info ) - 2, c_white, string_format( "%s",
                            c_diary->get_desc_map()[selected[window_mode::CHANGE_WIN]] ) );
        }
        bool debug = false;
        if( debug ) {
            if( currwin == window_mode::TEXT_WIN ) {
                auto text = c_diary->get_page_text();
                auto folded = foldstring( text, getmaxy( w_text ) - 1 );
                fold_and_print( w_info, point_south_east, getmaxx( w_info ) - 2, c_white,
                                string_format( "size: %s and wight: %s", folded[selected[window_mode::TEXT_WIN]].size(),
                                               utf8_wrapper( folded[selected[window_mode::TEXT_WIN]] ).display_width() ) );
            }
        }

        center_print( w_pages, 0, c_light_gray, string_format( _( "pages: %d" ),
                      c_diary->get_pages_list().size() ) );

        wnoutrefresh( w_diary );
        wnoutrefresh( w_border );
        wnoutrefresh( w_head );
        wnoutrefresh( w_pages );
        wnoutrefresh( w_changes );
        wnoutrefresh( w_text );
        wnoutrefresh( w_desc );
        wnoutrefresh( w_info );

    } );

    while( true ) {

        if( ( !c_diary->pages.empty() &&
              selected[window_mode::PAGE_WIN] >= static_cast<int>( c_diary->pages.size() ) ) ||
            ( c_diary->pages.empty() && selected[window_mode::PAGE_WIN] != 0 ) ) {
            selected[window_mode::PAGE_WIN] = 0;
        }
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "RIGHT" ) {
            currwin = static_cast<window_mode>( static_cast<int>( currwin ) + 1 );
            if( currwin >= window_mode::NUM_WIN ) {
                currwin = window_mode::FIRST_WIN;
            }
            selected[window_mode::TEXT_WIN] = 0;
        } else if( action == "LEFT" ) {
            currwin = static_cast<window_mode>( static_cast<int>( currwin ) - 1 );
            if( currwin < window_mode::FIRST_WIN ) {
                currwin = window_mode::LAST_WIN;
            }
            selected[window_mode::TEXT_WIN] = 0;
        } else if( action == "DOWN" ) {
            selected[currwin]++;
            if( selected[window_mode::PAGE_WIN] >= static_cast<int>( c_diary->pages.size() ) ) {
                selected[window_mode::PAGE_WIN] = 0;
            }
        } else if( action == "UP" ) {
            selected[currwin]--;
            if( selected[window_mode::PAGE_WIN] < 0 ) {
                selected[window_mode::PAGE_WIN] = c_diary->pages.empty() ? 0 : c_diary->pages.size() - 1;
            }

        } else if( action == "CONFIRM" ) {
            if( !c_diary->pages.empty() ) {
                c_diary->edit_page_ui( w_text );
            }    
        } else if( action == "NEW_PAGE" ) {
            c_diary->new_page();
            selected[window_mode::PAGE_WIN] = c_diary->pages.size() - 1;

        } else if( action == "DELETE PAGE" ) {
            if( !c_diary->pages.empty() ) {
                if( query_yn( _( "Really delete Page?" ) ) ) {
                    c_diary->delete_page();
                    if( selected[window_mode::PAGE_WIN] >= static_cast<int>( c_diary->pages.size() ) ) {
                        selected[window_mode::PAGE_WIN] --;
                    }
                }
            }
        } else if( action == "EXPORT_Diary" ) {
            if( query_yn( _( "Export Diary as .txt?" ) ) ) {
                c_diary->export_to_txt();
            }

        } else if( action == "QUIT" ) {
            break;
        }

    }
}
//isn´t needed anymore, because of string_editor_window edition
void diary::edit_page_ui()
{
    std::string title = _( "Text:" );
    static constexpr int max_note_length = 20000;
    
    const std::string old_text = get_page_ptr()->m_text;
    std::string new_text = old_text;

    bool esc_pressed = false;
    string_input_popup input_popup;
    input_popup
    .title( title )
    .width( max_note_length )
    .text( new_text )
    .description( "What happend Today?" )
    .title_color( c_white )
    .desc_color( c_light_gray )
    .string_color( c_yellow )
    .identifier( "diary" );


    do {
        new_text = input_popup.query_string( false );
        if( input_popup.canceled() ) {
            new_text = old_text;
            esc_pressed = true;
            break;
        } else if( input_popup.confirmed() ) {
            break;
        }

    } while( true );

    if( !esc_pressed && new_text.empty() && !old_text.empty() ) {
        if( query_yn( _( "Really delete note?" ) ) ) {
            get_page_ptr()->m_text = "";
        }
    } else if( !esc_pressed && old_text != new_text ) {
        get_page_ptr()->m_text = new_text;
    }

}

void diary::edit_page_ui( catacurses::window &win )
{
    const std::string old_text = get_page_ptr()->m_text;
    std::string new_text = old_text;

    string_editor_window ed = string_editor_window( win, get_page_ptr()->m_text );

    new_text = ed.query_string( true );

    if( new_text.empty() && !old_text.empty() ) {
        if( query_yn( _( "Really delete note?" ) ) ) {
            get_page_ptr()->m_text = "";
        }
    } else if( old_text != new_text ) {
        if( query_yn( _( "Save entry?" ) ) ) {
            get_page_ptr()->m_text = new_text;
        }
    }
}


