#include "UIHandler.h"

#include "debug.h"

UIWindow::UIWindow( int minSizeX, int minSizeY, Location location,
                    bool drawBorder ) : m_panel( new UIPaddingPanel( drawBorder ) )
{
    m_minSize.x = minSizeX;
    m_minSize.y = minSizeY;

    m_thisLocation = location;

    UpdateWindowSize();
}

void UIWindow::UpdateWindowSize()
{
    assert( m_panel );
    m_thisSize = m_panel->RequestedSize( Sizes::Prefered );

    point minSize = m_panel->RequestedSize( Sizes::Minimum );

    m_thisSize.x = std::max( m_thisSize.x, m_minSize.x );
    m_thisSize.y = std::max( m_thisSize.y, m_minSize.y );

    m_thisSize.x = std::min( m_thisSize.x, TERMX );
    m_thisSize.y = std::min( m_thisSize.y, TERMY );

    if( m_minSize.x > TERMX ) {
        DebugLog( D_ERROR, DC_ALL ) << "Window's Min Size is greater than terminal's. (X) Window: " <<
                                    m_minSize.x << " Term: " << TERMX;
    }
    if( m_minSize.y > TERMY ) {
        DebugLog( D_ERROR, DC_ALL ) << "Window's Min Size is greater than terminal's. (Y) Window: " <<
                                    m_minSize.y << " Term: " << TERMY;
    }

    if( minSize.x > TERMX ) {
        DebugLog( D_ERROR, DC_ALL ) <<
                                    "Window's child panel's Min Size is greater than terminal's. (X) Window: " << minSize.x << " Term: "
                                    << TERMX;
    }
    if( minSize.y > TERMY ) {
        DebugLog( D_ERROR, DC_ALL ) <<
                                    "Window's child panel's Min Size is greater than terminal's. (Y) Window: " << minSize.y << " Term: "
                                    << TERMY;
    }

    m_panel->SetSize( m_thisSize );

    switch( m_thisLocation ) {
        case Location::Centered:
            m_offset.x = ( TERMX - m_thisSize.x ) / 2;
            m_offset.y = ( TERMY - m_thisSize.y ) / 2;
            break;
    }

    m_wf_win = newwin( m_thisSize.y, m_thisSize.x, m_offset.y, m_offset.x );
    m_wf_winptr = WINDOW_PTR( m_wf_win );
    return;
}

void UIWindow::DrawEverything()
{
    werase( m_wf_win );
    assert( m_panel );
    m_panel->DrawEverything( m_wf_win, { 0, 0 } );
}

std::shared_ptr<UIPanel> UIPaddingPanel::GetChild() const
{
    return m_childPanel;
}

void UIPaddingPanel::SetChild( std::shared_ptr<UIPanel> panel )
{
    m_childPanel = panel;
}

point UIPaddingPanel::RequestedSize( Sizes sizes )
{
    point size;

    // Space for tab's border
    if( m_drawBorder )
        size += { 2, 2 };

    if( m_childPanel == nullptr ) {
        return size;
    }

    size += m_childPanel->RequestedSize( sizes );

    return size;
}

// We are a simple border!
void UIPaddingPanel::SetSize( point size )
{
    m_thisSize = size;

    if( m_childPanel == nullptr ) {
        return;
    }
    auto newSize = size;

    if( m_drawBorder ) //they lose two tiles for the tabs
        newSize -= { 2, 2 };

    m_childPanel->SetSize( newSize );
    return;
}

void UIPaddingPanel::DrawEverything( WINDOW *wf_win, point offset )
{

    if( m_drawBorder ) {
        UIUtils::DrawBorder( wf_win, offset, m_thisSize );
    }

    if( m_childPanel != nullptr ) {
        point offset;

        if( m_drawBorder ) // They should go into the border, not on it
            offset += { 1, 1 };

        m_childPanel->DrawEverything( wf_win, offset );
    }
}


UIPaddingPanel::UIPaddingPanel( bool drawBorder )
{
    m_drawBorder = drawBorder;
}

std::vector<std::pair<std::string, std::shared_ptr<UIPanel>>> UITabPanel::GetTabs() const
{
    return m_childPanels;
}

void UITabPanel::AddTab( std::string name, std::shared_ptr<UIPanel> panel )
{
    m_childPanels.push_back( std::pair<std::string, std::shared_ptr<UIPanel>>( name, panel ) );
}

void UITabPanel::RemoveTab( size_t index )
{
    assert( m_childPanels.size() > index );
    m_childPanels.erase( m_childPanels.begin() + index );
}

point UITabPanel::RequestedSize( Sizes sizes )
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

    auto paneSize = m_childPanels[m_currentTab].second->RequestedSize( sizes );
    size += paneSize;

    int len = 0;
    for( auto tx : m_childPanels ) {
        len += utf8_width( tx.first ) + 6; // 6 tiles for the ".<||>."
    }

    size.x = std::max( len, size.x );

    return size;
}

void UITabPanel::SetSize( point size )
{
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

void UITabPanel::DrawEverything( WINDOW *wf_win, point offset )
{
    if( m_drawBorder ) {
        auto bOffset = offset;

        //Acounting for the fact we have tabs
        bOffset += { 0, 2 }; // Go bellow the tabs

        auto bSize = m_thisSize;
        bSize -= { 0, 2 }; // We lose two characters to the tabs

        UIUtils::DrawBorder( wf_win, bOffset, bSize );
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

        UIUtils::DrawTab( wf_win, offset, toffset, ( i == m_currentTab ), m_childPanels[i].first );

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

void UITabPanel::SwitchTab( size_t tab )
{
    m_currentTab = tab;

    // Regenerate size for children
    SetSize( m_thisSize );
}

UITabPanel::UITabPanel( bool drawBorder )
{
    m_drawBorder = drawBorder;
}

void UIUtils::DrawBorder( WINDOW *wf_win, point offset, point m_thisSize )
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

void UIUtils::DrawTab( WINDOW *wf_win, point offset, int tabOffset, bool tabActive,
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
