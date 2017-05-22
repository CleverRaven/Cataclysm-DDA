#include "UIHandler.h"

#include "debug.h"

ui::window::window( int min_size_x, int min_size_y, location new_location,
                    bool new_draw_border ) : main_panel( new PaddingPanel( new_draw_border ) )
{
    min_size.x = min_size_x;
    min_size.y = min_size_y;

    this_location = new_location;

    UpdateWindowSize();
}

void ui::window::UpdateWindowSize()
{
    assert( main_panel );
    this_size = main_panel->RequestedSize( sizes::PREFERED );

    point panelMinSize = main_panel->RequestedSize( sizes::MINIMUM );

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

    if( panelMinSize.x > TERMX ) {
        DebugLog( D_ERROR, DC_ALL ) <<
                                    "Window's child panel's Min Size is greater than terminal's. (X) Window: " << panelMinSize.x
                                    << " Term: " << TERMX;
    }
    if( panelMinSize.y > TERMY ) {
        DebugLog( D_ERROR, DC_ALL ) <<
                                    "Window's child panel's Min Size is greater than terminal's. (Y) Window: " << panelMinSize.y
                                    << " Term: " << TERMY;
    }

    main_panel->SetSize( this_size );

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

void ui::window::DrawEverything()
{
    assert( wf_win );
    werase( wf_win );
    assert( main_panel );
    main_panel->DrawEverything( wf_win, { 0, 0 } );
}

std::shared_ptr<ui::Panel> ui::PaddingPanel::GetChild() const
{
    return childPanel;
}

void ui::PaddingPanel::SetChild( std::shared_ptr<Panel> child_panel )
{
    childPanel = child_panel;
}

point ui::PaddingPanel::RequestedSize( sizes size )
{
    point req_size;

    // Space for tab's border
    if( drawBorder )
        req_size += { 2, 2 };

    if( childPanel == nullptr ) {
        return req_size;
    }

    req_size += childPanel->RequestedSize( size );

    return req_size;
}

// We are a simple border!
void ui::PaddingPanel::SetSize( point size )
{
    point reqSize = RequestedSize( sizes::MINIMUM );
    assert( size.x >= reqSize.x );
    assert( size.y >= reqSize.y );

    this_size = size;

    if( childPanel == nullptr ) {
        return;
    }
    auto newSize = size;

    if( drawBorder ) //they lose two tiles for the tabs
        newSize -= { 2, 2 };

    childPanel->SetSize( newSize );
    return;
}

void ui::PaddingPanel::DrawEverything( WINDOW *wf_win, point offset )
{

    if( drawBorder ) {
        utils::DrawBorder( wf_win, offset, this_size );
    }

    if( childPanel != nullptr ) {
        point offset;

        if( drawBorder ) // They should go into the border, not on it
            offset += { 1, 1 };

        childPanel->DrawEverything( wf_win, offset );
    }
}


ui::PaddingPanel::PaddingPanel( bool new_draw_border )
{
    drawBorder = new_draw_border;
}

std::vector<std::pair<std::string, std::shared_ptr<ui::Panel>>> ui::TabPanel::GetTabs() const
{
    return childPanels;
}

void ui::TabPanel::AddTab( std::string name, std::shared_ptr<Panel> tab_panel )
{
    childPanels.push_back( std::pair<std::string, std::shared_ptr<Panel>>( name, tab_panel ) );
}

void ui::TabPanel::RemoveTab( size_t index )
{
    assert( childPanels.size() > index );
    childPanels.erase( childPanels.begin() + index );
}

point ui::TabPanel::RequestedSize( sizes sizes )
{
    point req_size;

    // Space for the tab's height
    req_size += { 0, 2 };

    if( drawBorder )
        // Panel's border
        req_size += { 2, 2 };

    if( childPanels.empty() ) {
        return req_size;
    }

    assert( currentTab < childPanels.size() );
    assert( childPanels[currentTab].second != nullptr );

    req_size += childPanels[currentTab].second->RequestedSize( sizes );

    int len = 0;
    for( auto tx : childPanels ) {
        len += utf8_width( tx.first ) + 6; // 6 tiles for the ".<||>."
    }

    req_size.x = std::max( len, req_size.x );

    return req_size;
}

void ui::TabPanel::SetSize( point size )
{
    point reqSize = RequestedSize( sizes::MINIMUM );
    assert( size.x >= reqSize.x );
    assert( size.y >= reqSize.y );

    this_size = size;

    if( childPanels.empty() ) {
        return;
    }
    auto newSize = size;

    if( drawBorder ) // they lose two characters to the border
        newSize -= { 2, 2 };

    newSize -= { 0, 2 }; // and another two (on the y) for the tabs

    childPanels[currentTab].second->SetSize( newSize );
    return;
}

void ui::TabPanel::DrawEverything( WINDOW *wf_win, point offset )
{
    if( drawBorder ) {
        auto bOffset = offset;

        //Acounting for the fact we have tabs
        bOffset += { 0, 2 }; // Go bellow the tabs

        auto bSize = this_size;
        bSize -= { 0, 2 }; // We lose two characters to the tabs

        utils::DrawBorder( wf_win, bOffset, bSize );
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

    int toffset = 0;
    for( size_t i = 0; i < childPanels.size(); i++ ) {
        toffset += 1;

        utils::DrawTab( wf_win, offset, toffset, ( i == currentTab ), childPanels[i].first );

        toffset += 5 + utf8_width( childPanels[i].first );
    }

    if( !childPanels.empty() ) {
        point offset;

        if( drawBorder ) // if we have borders we want to go in them'
            offset += { 1, 1 };

        offset += { 0, 2 }; // We go bellow the tabs

        childPanels[currentTab].second->DrawEverything( wf_win, offset );
    }
}

void ui::TabPanel::SwitchTab( size_t tab )
{
    currentTab = tab;

    // Regenerate size for children
    SetSize( this_size );
}

ui::TabPanel::TabPanel( bool new_draw_border )
{
    drawBorder = new_draw_border;
}

void ui::utils::DrawBorder( WINDOW *wf_win, point offset, point this_size )
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

void ui::utils::DrawTab( WINDOW *wf_win, point offset, int tabOffset, bool tabActive,
                         std::string text )
{
    int tabOffsetRight = tabOffset + utf8_width( text ) + 1;

    mvwputch( wf_win, offset.y, offset.x + tabOffset, c_ltgray, LINE_OXXO );          // |^
    mvwputch( wf_win, offset.y, offset.x + tabOffsetRight, c_ltgray, LINE_OOXX );     // ^|
    mvwputch( wf_win, offset.y + 1, offset.x + tabOffset, c_ltgray, LINE_XOXO );      // |
    mvwputch( wf_win, offset.y + 1, offset.x + tabOffsetRight, c_ltgray, LINE_XOXO ); // |

    mvwprintz( wf_win, offset.y + 1, offset.x + tabOffset + 1, ( tabActive ) ? h_ltgray : c_ltgray,
               "%s", text.c_str() );

    for( int i = tabOffset + 1; i < tabOffsetRight; i++ ) {
        mvwputch( wf_win, offset.y, offset.x + i, c_ltgray, LINE_OXOX ); // -
    }

    if( tabActive ) {
        mvwputch( wf_win, offset.y + 1, offset.x + tabOffset - 1, h_ltgray, '<' );
        mvwputch( wf_win, offset.y + 1, offset.x + tabOffsetRight + 1, h_ltgray, '>' );

        for( int i = tabOffset + 1; i < tabOffsetRight; i++ ) {
            mvwputch( wf_win, offset.y + 2, offset.x + i, c_black, ' ' );
        }

        mvwputch( wf_win, offset.y + 2, offset.x + tabOffset,      c_ltgray, LINE_XOOX ); // _|
        mvwputch( wf_win, offset.y + 2, offset.x + tabOffsetRight, c_ltgray, LINE_XXOO ); // |_

    } else {
        mvwputch( wf_win, offset.y + 2, offset.x + tabOffset,      c_ltgray, LINE_XXOX ); // _|_
        mvwputch( wf_win, offset.y + 2, offset.x + tabOffsetRight, c_ltgray, LINE_XXOX ); // _|_
    }
}
