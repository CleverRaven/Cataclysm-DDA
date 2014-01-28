#include "construction_ui.h"
#include "game.h"
#include "keypress.h"
#include <string>

// Construction UI

void construct_ui::display( player *p ) {
    init( p );

    WINDOW *head = newwin( head_height, w_width, headstart, colstart );
    WINDOW *listing = newwin( w_height, l_width, headstart + head_height, colstart );
    WINDOW *description = newwin( w_height, w_width - l_width, headstart + head_height, colstart + l_width );

    while( !exit ) {
        draw_header( head );
        draw_list( listing );
        draw_description( description );
        handle_key( getch() );
    }

    werase( head );
    werase( listing );
    werase( description );
    delwin( head );
    delwin( listing );
    delwin( description );
    g->refresh_all( );
}

void construct_ui::init( player *p ) {
    this->u = p;

    load_constructions( constructions.begin(), constructions.end() );

    w_height = ( TERMY < min_w_height + head_height ) ? min_w_height : ( TERMY > max_w_height + head_height ) ? max_w_height : (int)TERMY - head_height;
    w_width = ( TERMX < min_w_width ) ? min_w_width : ( TERMX > max_w_width ) ? max_w_width : (int)TERMX;
    l_width = ( w_width / 2 > max_l_width ) ? max_l_width : ( w_width / 2 < min_l_width ) ? min_l_width : w_width / 2;

    headstart = ( TERMY > w_height + head_height ) ? ( TERMY - w_height ) / 2 : 0;
    colstart = ( TERMX > w_width ) ? ( TERMX - w_width ) / 2 : 0;
}

void construct_ui::handle_key( int key ) {
    switch( key ) {
        case KEY_ESCAPE:
            exit = true;
            break;
        case '\t':
            filter++;
            redraw_flags = ALL_WIN;
            break;
        case KEY_DOWN:
            selected_index++;
            if( selected_index > items_size ) {
                selected_index = 0;
            }
            redraw_flags = BODY;
            break;
        case KEY_UP:
            selected_index--;
            if( selected_index < 0 ) {
                selected_index = items_size;
            }
            redraw_flags = BODY;
            break;
    }
}

void construct_ui::draw_header( WINDOW *win ) {
    if( ( redraw_flags & HEAD ) != HEAD ) { return; }
    redraw_flags ^= HEAD;

    werase( win );
    draw_border( win );

    const char *title = _( " Construction " );
    mvwprintz( win, 0, ( w_width / 2 ) - ( utf8_width( title ) / 2 ), c_ltred, title );

    // print filter settings
    mvwprintz( win, 1, 1, c_white, _( "Press tab to cycle the filter between all, available, or unavailable." ) );
    mvwprintz( win, 2, 1, c_white, _( "Showing %s constructions." ), get_filter_text().c_str() );

    wrefresh( win );
}

void construct_ui::draw_list( WINDOW *win ) {
    if( ( redraw_flags & LIST ) != LIST ) { return; }
    redraw_flags ^= LIST;

    werase( win );
    draw_border( win );

    std::set<construct_ui_item, construction_sort_functor> items = get_filtered_items( );

    if( items.size() <= 0 ) {
        selected_index = -1;
        selected_item = NULL;
        wrefresh( win );
        return;
    }

    selected_index = selected_index < 0 ? 0 : selected_index;

    if( items.size() > w_height - 1) {
        draw_scrollbar( win, selected_index, w_height - 2, items.size(), 1 );
    }

    int line = 0;
    int offset = 0;

    if( selected_index >= w_height - 2 ) {
        offset = selected_index - w_height + 3;
    }

    sort_it start = items.begin();
    if( offset > 0 ) {
        std::advance( start, offset );
    }

    for( sort_it it = start; it != items.end( ) && line < w_height - 2; ++it ) {
        nc_color color = it->available ? c_white : c_dkgray;
        if( line + offset == selected_index ) {
            color = hilite( color );
            selected_item = &(*it);
        }

        mvwprintz( win, ++line, 1, color, it->display_name( l_width - 3 ).c_str( ) );
    }

    wrefresh( win );
}

void construct_ui::draw_description( WINDOW *win ) {
    if( ( redraw_flags & DESCRIPTION ) != DESCRIPTION ) { return; }
    redraw_flags ^= DESCRIPTION;

    werase( win );
    draw_border( win );

    if( selected_item == NULL) {
        wrefresh( win );
        return;
    }

    int line = 0;
    mvwprintz( win, ++line, 1, c_white, "%s", selected_item->full_name().c_str());

    for( con_iter it = selected_item->begin(); it != selected_item->end(); ++it ) {
        construction *c = ( *it );

        line = print_construction( win, line, c );
        line++;
    }

    wrefresh( win );
}

int construct_ui::print_construction( WINDOW *win, int line, construction *c ) {
    int skill = u->skillLevel( c->skill );
    int diff = c->difficulty > 0 ? c->difficulty : 0;

    mvwprintz( win, line++, 1, c_white, _( "Skill: %s" ), Skill::skill( c->skill )->name( ).c_str( ) );
    mvwprintz( win, line++, 1, ( skill >= diff ? c_white : c_red ), _( "Difficulty: %d" ), diff );

    if( c->pre_terrain != "" ) {
        mvwprintz( win, line++, 1, c_white, _( "Replaces: %s" ), c->get_pre_terrain_name().c_str() );
    }

    if( c->post_terrain != "" ) {
        mvwprintz( win, line++, 1, c_white, _( "Result: %s" ), c->get_post_terrain_name().c_str( ) );
    }

    mvwprintz( win, line++, 1, c_white, _( "Time: %1d minutes" ), c->time );


    return line;
}

void construct_ui::load_constructions( con_iter begin, con_iter end ) {
    for( con_iter it = begin; it < end; ++it ) {
        construction *c = ( *it );
        cui_iter iter = construct_ui::construct_ui_items.find( c->description );

        if( iter == construct_ui::construct_ui_items.end( ) ) {
            iter = ( construct_ui::construct_ui_items.insert( std::make_pair( c->description, construct_ui::construct_ui_item( c->description ) ) ) ).first;
        }

        iter->second.add( c );
    }
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

std::set<construct_ui::construct_ui_item, construct_ui::construction_sort_functor> construct_ui::get_filtered_items( ) {
    std::set<construct_ui::construct_ui_item, construct_ui::construction_sort_functor> items;
    inventory craft_inv = g->crafting_inventory( u );

    for( cui_iter it = construct_ui_items.begin(); it != construct_ui_items.end(); ++it ) {
        bool available = it->second.can_construct( *u, craft_inv );

        if( filter == ALL || (filter == AVAILABLE && available) || (filter == UNAVAILABLE && !available)) {
            items.insert( it->second );
        }
    }

    items_size = items.size() - 1;

    return items;
}

void construct_ui::construct_ui_item::add( construction* c ) {
    constructions.push_back( c );
}

std::string construct_ui::construct_ui_item::display_name( int width ) const {
    return utf8_substr( name, 0, width );
}

std::string construct_ui::construct_ui_item::full_name() const {
    return name;
}

construct_ui::con_iter construct_ui::construct_ui_item::begin( ) const {
    return this->constructions.begin();
}

construct_ui::con_iter construct_ui::construct_ui_item::end( ) const {
    return this->constructions.end( );
}

bool construct_ui::construct_ui_item::can_construct(player &p, inventory craft_inv, bool use_cache) {
    if( use_cache && cached ) { return available; }
    cached = true;
    for( con_iter it = this->constructions.begin(); it != this->constructions.end(); ++it ) {
        if( ( *it )->can_construct() && ( *it )->player_can_build( p, craft_inv ) ) {
            return available = true;
        }
    }
    return available = false;
}
