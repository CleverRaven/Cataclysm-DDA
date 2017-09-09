#include "UIHandler.h"

#include "debug.h"

ui::window::window( int min_size_x, int min_size_y, location new_location,
                    bool new_draw_border ) : main_panel( new padding_panel( new_draw_border ) )
{
    min_size.x = min_size_x;
    min_size.y = min_size_y;

    this_location = new_location;

    update_window_size();
}

void ui::window::update_window_size()
{
    assert( main_panel );
    this_size = main_panel->requested_size( sizes::PREFERED );

    point panel_min_size = main_panel->requested_size( sizes::MINIMUM );

    this_size.x = std::max( this_size.x, min_size.x );
    this_size.y = std::max( this_size.y, min_size.y );

    this_size.x = std::min( this_size.x, TERMX );
    this_size.y = std::min( this_size.y, TERMY );

    if( min_size.x > TERMX ) {
        DebugLog( D_ERROR, DC_ALL ) << "Window's Min Size is greater than terminal's. (X) Window: "
                                    << min_size.x << " Term: " << TERMX;
    }
    if( min_size.y > TERMY ) {
        DebugLog( D_ERROR, DC_ALL ) << "Window's Min Size is greater than terminal's. (Y) Window: "
                                    << min_size.y << " Term: " << TERMY;
    }

    if( panel_min_size.x > TERMX ) {
        DebugLog( D_ERROR, DC_ALL ) <<
                                    "Window's child panel's Min Size is greater than terminal's. (X) Window: " << panel_min_size.x
                                    << " Term: " << TERMX;
    }
    if( panel_min_size.y > TERMY ) {
        DebugLog( D_ERROR, DC_ALL ) <<
                                    "Window's child panel's Min Size is greater than terminal's. (Y) Window: " << panel_min_size.y
                                    << " Term: " << TERMY;
    }

    main_panel->set_size( this_size );

    switch( this_location ) {
        case location::CENTERED:
            offset.x = ( TERMX - this_size.x ) / 2;
            offset.y = ( TERMY - this_size.y ) / 2;
            break;
    }

    wf_win = newwin( this_size.y, this_size.x, offset.y, offset.x );
    assert( wf_win );
    wf_winptr = WINDOW_PTR( wf_win );
    return;
}

void ui::window::draw_everything()
{
    assert( wf_win );
    werase( wf_win );
    assert( main_panel );
    main_panel->draw_everything( wf_win, { 0, 0 } );
}

std::shared_ptr<ui::panel> ui::padding_panel::get_child() const
{
    return child_panel;
}

void ui::padding_panel::set_child( std::shared_ptr<panel> new_child_panel )
{
    child_panel = new_child_panel;
}

point ui::padding_panel::requested_size( sizes size )
{
    point req_size;

    // Space for tab's border
    if( draw_border )
        req_size += { 2, 2 };

    if( child_panel == nullptr ) {
        return req_size;
    }

    req_size += child_panel->requested_size( size );

    return req_size;
}

void ui::padding_panel::set_size( point size )
{
    point req_size = requested_size( sizes::MINIMUM );
    assert( size.x >= req_size.x );
    assert( size.y >= req_size.y );

    this_size = size;

    if( child_panel == nullptr ) {
        return;
    }
    auto new_size = size;

    if( draw_border ) //they lose two tiles for the tabs
        new_size -= { 2, 2 };

    child_panel->set_size( new_size );
    return;
}

void ui::padding_panel::draw_everything( WINDOW *wf_win, point offset )
{

    if( draw_border ) {
        utils::draw_border( wf_win, offset, this_size );
    }

    if( child_panel != nullptr ) {
        point offset;

        if( draw_border ) // They should go into the border, not on it
            offset += { 1, 1 };

        child_panel->draw_everything( wf_win, offset );
    }
}


ui::padding_panel::padding_panel( bool new_draw_border )
{
    draw_border = new_draw_border;
}

std::vector<std::pair<std::string, std::shared_ptr<ui::panel>>> ui::tab_panel::get_tabs() const
{
    return child_panels;
}

void ui::tab_panel::add_tab( std::string name, std::shared_ptr<panel> tab_panel )
{
    child_panels.push_back( std::pair<std::string, std::shared_ptr<panel>>( name, tab_panel ) );
}

void ui::tab_panel::remove_tab( size_t index )
{
    assert( child_panels.size() > index );
    child_panels.erase( child_panels.begin() + index );
}

point ui::tab_panel::requested_size( sizes size )
{
    point req_size;

    // Space for the tab's height
    req_size += { 0, 2 };

    if( draw_border )
        // panel's border
        req_size += { 2, 2 };

    if( child_panels.empty() ) {
        return req_size;
    }

    assert( current_tab < child_panels.size() );
    assert( child_panels[current_tab].second != nullptr );

    req_size += child_panels[current_tab].second->requested_size( size );

    int len = 0;
    for( auto tx : child_panels ) {
        len += utf8_width( tx.first ) + 6; // 6 tiles for the ".<||>."
    }

    req_size.x = std::max( len, req_size.x );

    return req_size;
}

void ui::tab_panel::set_size( point size )
{
    point req_size = requested_size( sizes::MINIMUM );
    assert( size.x >= req_size.x );
    assert( size.y >= req_size.y );

    this_size = size;

    if( child_panels.empty() ) {
        return;
    }
    auto new_size = size;

    if( draw_border ) // they lose two characters to the border
        new_size -= { 2, 2 };

    new_size -= { 0, 2 }; // and another two (on the y) for the tabs

    child_panels[current_tab].second->set_size( new_size );
    return;
}

void ui::tab_panel::draw_everything( WINDOW *wf_win, point offset )
{
    if( draw_border ) {
        auto b_offset = offset;

        //Acounting for the fact we have tabs
        b_offset += { 0, 2 }; // Go bellow the tabs

        auto b_size = this_size;
        b_size -= { 0, 2 }; // We lose two characters to the tabs

        utils::draw_border( wf_win, b_offset, b_size );
    }

    //We add 1 so we get one space to the left of the tab
    //v
    //.<|TEXT|>..<|TEXT|>.
    //.^
    //We then draw our tab
    //      We then add the length of the text (4 in this case)
    //.....V
    //.<|TEXT|>..<|TEXT|>.
    //..........^
    //We then add 5 to account for the "<||>."

    int t_offset = 0;
    for( size_t i = 0; i < child_panels.size(); i++ ) {
        t_offset += 1;

        utils::draw_tab( wf_win, offset, t_offset, ( i == current_tab ), child_panels[i].first );

        t_offset += 5 + utf8_width( child_panels[i].first );
    }

    if( !child_panels.empty() ) {
        point offset;

        if( draw_border ) // if we have borders we want to go in them'
            offset += { 1, 1 };

        offset += { 0, 2 }; // We go bellow the tabs

        child_panels[current_tab].second->draw_everything( wf_win, offset );
    }
}

void ui::tab_panel::switch_tab( size_t tab )
{
    current_tab = tab;

    // Regenerate size for children
    set_size( this_size );
}

ui::tab_panel::tab_panel( bool new_draw_border )
{
    draw_border = new_draw_border;
}

void ui::utils::draw_border( WINDOW *wf_win, point offset, point this_size )
{
    // Bottom and top border
    for( int i = 1; i < this_size.x - 1; i++ ) {
        mvwputch( wf_win, offset.y, i + offset.x, BORDER_COLOR, LINE_OXOX );
        mvwputch( wf_win, offset.y + this_size.y - 1, i + offset.x, BORDER_COLOR, LINE_OXOX );
    }

    // Right and left border
    for( int i = 1; i < this_size.y - 1; i++ ) {
        mvwputch( wf_win, i + offset.y, offset.x, BORDER_COLOR, LINE_XOXO );
        mvwputch( wf_win, i + offset.y, offset.x + this_size.x - 1, BORDER_COLOR, LINE_XOXO );
    }

    // Corners
    mvwputch( wf_win, offset.y, offset.x, BORDER_COLOR,
              LINE_OXXO );                                         // |^
    mvwputch( wf_win, offset.y, offset.x + this_size.x - 1, BORDER_COLOR,
              LINE_OOXX );                      // ^|
    mvwputch( wf_win, offset.y + this_size.y - 1, offset.x, BORDER_COLOR,
              LINE_XXOO );                      // |_
    mvwputch( wf_win, offset.y + this_size.y - 1, offset.x + this_size.x - 1, BORDER_COLOR,
              LINE_XOOX ); // _|
}

void ui::utils::draw_tab( WINDOW *wf_win, point offset, int tab_offset, bool tab_active,
                         std::string text )
{
    int tab_offset_right = tab_offset + utf8_width( text ) + 1;

    mvwputch( wf_win, offset.y, offset.x + tab_offset, c_ltgray, LINE_OXXO );          // |^
    mvwputch( wf_win, offset.y, offset.x + tab_offset_right, c_ltgray, LINE_OOXX );     // ^|
    mvwputch( wf_win, offset.y + 1, offset.x + tab_offset, c_ltgray, LINE_XOXO );      // |
    mvwputch( wf_win, offset.y + 1, offset.x + tab_offset_right, c_ltgray, LINE_XOXO ); // |

    mvwprintz( wf_win, offset.y + 1, offset.x + tab_offset + 1, ( tab_active ) ? h_ltgray : c_ltgray,
               "%s", text.c_str() );

    for( int i = tab_offset + 1; i < tab_offset_right; i++ ) {
        mvwputch( wf_win, offset.y, offset.x + i, c_ltgray, LINE_OXOX ); // -
    }

    if( tab_active ) {
        mvwputch( wf_win, offset.y + 1, offset.x + tab_offset - 1, h_ltgray, '<' );
        mvwputch( wf_win, offset.y + 1, offset.x + tab_offset_right + 1, h_ltgray, '>' );

        for( int i = tab_offset + 1; i < tab_offset_right; i++ ) {
            mvwputch( wf_win, offset.y + 2, offset.x + i, c_black, ' ' );
        }

        mvwputch( wf_win, offset.y + 2, offset.x + tab_offset,      c_ltgray, LINE_XOOX ); // _|
        mvwputch( wf_win, offset.y + 2, offset.x + tab_offset_right, c_ltgray, LINE_XXOO ); // |_

    } else {
        mvwputch( wf_win, offset.y + 2, offset.x + tab_offset,      c_ltgray, LINE_XXOX ); // _|_
        mvwputch( wf_win, offset.y + 2, offset.x + tab_offset_right, c_ltgray, LINE_XXOX ); // _|_
    }
}
