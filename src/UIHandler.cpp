#include "UIHandler.h"

#include "debug.h"

ui::Window::Window( int minSizeX, int minSizeY, Location location,
                    bool newDrawBorder ) : panel( new PaddingPanel( newDrawBorder ) )
{
    minSize.x = minSizeX;
    minSize.y = minSizeY;

    m_thisLocation = location;

    UpdateWindowSize();
}

void ui::Window::UpdateWindowSize()
{
    assert( panel );
    thisSize = panel->RequestedSize( Sizes::Prefered );

    point panelMinSize = panel->RequestedSize( Sizes::Minimum );

    thisSize.x = std::max( thisSize.x, minSize.x );
    thisSize.y = std::max( thisSize.y, minSize.y );

    thisSize.x = std::min( thisSize.x, TERMX );
    thisSize.y = std::min( thisSize.y, TERMY );

    if( minSize.x > TERMX ) {
        DebugLog( D_ERROR, DC_ALL ) << "Window's Min Size is greater than terminal's. (X) Window: "
                                    << minSize.x << " Term: " << TERMX;
    }
    if( minSize.y > TERMY ) {
        DebugLog( D_ERROR, DC_ALL ) << "Window's Min Size is greater than terminal's. (Y) Window: "
                                    << minSize.y << " Term: " << TERMY;
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

    panel->SetSize( thisSize );

    switch( m_thisLocation ) {
        case Location::Centered:
            offset.x = ( TERMX - thisSize.x ) / 2;
            offset.y = ( TERMY - thisSize.y ) / 2;
            break;
    }

    wf_win = newwin( thisSize.y, thisSize.x, offset.y, offset.x );
    assert( wf_win );
    wf_winptr = WINDOW_PTR( wf_win );
    return;
}

void ui::Window::DrawEverything()
{
    assert( wf_win );
    werase( wf_win );
    assert( panel );
    panel->DrawEverything( wf_win, { 0, 0 } );
}

std::shared_ptr<ui::Panel> ui::PaddingPanel::GetChild() const
{
    return childPanel;
}

void ui::PaddingPanel::SetChild( std::shared_ptr<Panel> panel )
{
    childPanel = panel;
}

point ui::PaddingPanel::RequestedSize( Sizes sizes )
{
    point size;

    // Space for tab's border
    if( drawBorder )
        size += { 2, 2 };

    if( childPanel == nullptr ) {
        return size;
    }

    size += childPanel->RequestedSize( sizes );

    return size;
}

// We are a simple border!
void ui::PaddingPanel::SetSize( point size )
{
    point reqSize = RequestedSize( Sizes::Minimum );
    assert( size.x >= reqSize.x );
    assert( size.y >= reqSize.y );

    thisSize = size;

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
        Utils::DrawBorder( wf_win, offset, thisSize );
    }

    if( childPanel != nullptr ) {
        point offset;

        if( drawBorder ) // They should go into the border, not on it
            offset += { 1, 1 };

        childPanel->DrawEverything( wf_win, offset );
    }
}


ui::PaddingPanel::PaddingPanel( bool newDrawBorder )
{
    drawBorder = newDrawBorder;
}

std::vector<std::pair<std::string, std::shared_ptr<ui::Panel>>> ui::TabPanel::GetTabs() const
{
    return m_childPanels;
}

void ui::TabPanel::AddTab( std::string name, std::shared_ptr<Panel> panel )
{
    m_childPanels.push_back( std::pair<std::string, std::shared_ptr<Panel>>( name, panel ) );
}

void ui::TabPanel::RemoveTab( size_t index )
{
    assert( m_childPanels.size() > index );
    m_childPanels.erase( m_childPanels.begin() + index );
}

point ui::TabPanel::RequestedSize( Sizes sizes )
{
    point size;

    // Space for the tab's height
    size += { 0, 2 };

    if( m_drawBorder )
        // Panel's border
        size += { 2, 2 };

    if( m_childPanels.empty() ) {
        return size;
    }

    assert( m_currentTab < m_childPanels.size() );
    assert( m_childPanels[m_currentTab].second != nullptr );

    auto paneSize = m_childPanels[m_currentTab].second->RequestedSize( sizes );
    size += paneSize;

    int len = 0;
    for( auto tx : m_childPanels ) {
        len += utf8_width( tx.first ) + 6; // 6 tiles for the ".<||>."
    }

    size.x = std::max( len, size.x );

    return size;
}

void ui::TabPanel::SetSize( point size )
{
    point reqSize = RequestedSize( Sizes::Minimum );
    assert( size.x >= reqSize.x );
    assert( size.y >= reqSize.y );

    m_thisSize = size;

    if( m_childPanels.empty() ) {
        return;
    }
    auto newSize = size;

    if( m_drawBorder ) // they lose two characters to the border
        newSize -= { 2, 2 };

    newSize -= { 0, 2 }; // and another two (on the y) for the tabs

    m_childPanels[m_currentTab].second->SetSize( newSize );
    return;
}

void ui::TabPanel::DrawEverything( WINDOW *wf_win, point offset )
{
    if( m_drawBorder ) {
        auto bOffset = offset;

        //Acounting for the fact we have tabs
        bOffset += { 0, 2 }; // Go bellow the tabs

        auto bSize = m_thisSize;
        bSize -= { 0, 2 }; // We lose two characters to the tabs

        Utils::DrawBorder( wf_win, bOffset, bSize );
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
    for( size_t i = 0; i < m_childPanels.size(); i++ ) {
        toffset += 1;

        Utils::DrawTab( wf_win, offset, toffset, ( i == m_currentTab ), m_childPanels[i].first );

        toffset += 5 + utf8_width( m_childPanels[i].first );
    }

    if( !m_childPanels.empty() ) {
        point offset;

        if( m_drawBorder ) // if we have borders we want to go in them'
            offset += { 1, 1 };

        offset += { 0, 2 }; // We go bellow the tabs

        m_childPanels[m_currentTab].second->DrawEverything( wf_win, offset );
    }
}

void ui::TabPanel::SwitchTab( size_t tab )
{
    m_currentTab = tab;

    // Regenerate size for children
    SetSize( m_thisSize );
}

ui::TabPanel::TabPanel( bool drawBorder )
{
    m_drawBorder = drawBorder;
}

void ui::Utils::DrawBorder( WINDOW *wf_win, point offset, point m_thisSize )
{
    // Bottom and top border
    for( int i = 1; i < m_thisSize.x - 1; i++ ) {
        mvwputch( wf_win, offset.y, i + offset.x, BORDER_COLOR, LINE_OXOX );
        mvwputch( wf_win, offset.y + m_thisSize.y - 1, i + offset.x, BORDER_COLOR, LINE_OXOX );
    }

    // Right and left border
    for( int i = 1; i < m_thisSize.y - 1; i++ ) {
        mvwputch( wf_win, i + offset.y, offset.x, BORDER_COLOR, LINE_XOXO );
        mvwputch( wf_win, i + offset.y, offset.x + m_thisSize.x - 1, BORDER_COLOR, LINE_XOXO );
    }

    // Corners
    mvwputch( wf_win, offset.y, offset.x, BORDER_COLOR,
              LINE_OXXO );                                         // |^
    mvwputch( wf_win, offset.y, offset.x + m_thisSize.x - 1, BORDER_COLOR,
              LINE_OOXX );                      // ^|
    mvwputch( wf_win, offset.y + m_thisSize.y - 1, offset.x, BORDER_COLOR,
              LINE_XXOO );                      // |_
    mvwputch( wf_win, offset.y + m_thisSize.y - 1, offset.x + m_thisSize.x - 1, BORDER_COLOR,
              LINE_XOOX ); // _|
}

void ui::Utils::DrawTab( WINDOW *wf_win, point offset, int tabOffset, bool tabActive,
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
