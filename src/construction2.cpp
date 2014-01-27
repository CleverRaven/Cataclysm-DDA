#include "construction2.h"
#include "game.h"
#include "keypress.h"
#include <string>

// Construction UI

void construct_ui::display( player *p ) {
    init(p);
    
    WINDOW *head = newwin( head_height, w_width, headstart, colstart );
    WINDOW *listing = newwin( w_height, l_width, headstart + head_height, colstart );
    WINDOW *description = newwin( w_height, w_width - l_width, headstart + head_height, colstart + l_width );

    while( !exit ) {
        draw_header( head );
        draw_list( listing );
        draw_description( description );
        handle_key( getch() );
    }
}

void construct_ui::init( player *p ) {
    this->u = p;
    
    w_height = ( TERMY < min_w_height + head_height ) ? min_w_height : (TERMY > max_w_height + head_height) ? max_w_height : (int)TERMY - head_height;
    w_width = ( TERMX < min_w_width ) ? min_w_width : ( TERMX > max_w_width ) ? max_w_width : (int) TERMX;
    l_width = ( w_width / 2 < min_l_width ) ? min_l_width : ( w_width / 2 > max_l_width ) ? max_l_width : w_width / 2;

    headstart = ( TERMY > w_height ) ? ( TERMY - w_height ) / 2 : 0;
    colstart = ( TERMX > w_width ) ? ( TERMX - w_width ) / 2 : 0;
}

void construct_ui::handle_key( int key ) {
    switch( key ) {
        case KEY_ESCAPE:
            exit = true;
            break;
    }
}

void construct_ui::draw_header( WINDOW *win ) {
    if( !redraw_head ) { return; }
    redraw_head = false;

    werase( win );
    draw_border( win );

    char *title = _( " Construction " );
    mvwprintz( win, 0, ( w_width / 2 ) - ( utf8_width( title ) / 2 ), c_ltred, title );
        
    // print filter settings
    mvwprintz( win, headstart, colstart, c_white, _( "Showing %s constructions." ), get_filter_text().c_str() );

    wrefresh( win );
}

void construct_ui::draw_list( WINDOW *win ) {
    if( !redraw_list ) { return; }
    redraw_list = false;

    werase( win );
    draw_border( win );

    wrefresh( win );
}

void construct_ui::draw_description( WINDOW *win ) {
    if( !redraw_description ) { return; }
    redraw_description = false;

    werase( win );
    draw_border( win );

    wrefresh( win );
}

std::string construct_ui::get_filter_text() {
    switch( filter ) {
        case AVAILABLE:
            return _( "available" );
        case UNAVAILABLE:
            return _( "unavailable" );
        case ALL:
        default:
            return _( "all" );
    }
}