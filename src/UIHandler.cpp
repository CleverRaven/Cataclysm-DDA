#include "UIHandler.h"

UIWindow::UIWindow( int minSizeX, int minSizeY, Location location,
                    bool drawBorder ) : m_panel( new UIPaddingPanel( drawBorder ) )
{
    m_minSize.x = minSizeX;
    m_minSize.y = minSizeY;

    m_thisLocation = location;

    UpdateWindow();
}

int UIWindow::UpdateWindow()
{
    // TODO: Clear prev contents? Is that our job?

    m_thisSize = m_panel->RequestedSize( Sizes::Prefered );

    point minSize = m_panel->RequestedSize( Sizes::Minimum );

    if( m_minSize.x > m_thisSize.x ) {
        m_thisSize.x = m_minSize.x;
    }
    if( m_minSize.y > m_thisSize.y ) {
        m_thisSize.y = m_minSize.y;
    }

    if( m_thisSize.x > TERMX ) {
        m_thisSize.x = TERMX;
    }
    if( m_thisSize.y > TERMY ) {
        m_thisSize.y = TERMY;
    }

    if( m_minSize.x > TERMX ) {
        DebugLog( D_ERROR, DC_ALL ) << "Not enought screen space...";
    }
    if( m_minSize.y > TERMY ) {
        DebugLog( D_ERROR, DC_ALL ) << "Not enought screen space...";
    }

    if( minSize.x > TERMX ) {
        DebugLog( D_ERROR, DC_ALL ) << "Not enought screen space...";
    }
    if( minSize.y > TERMY ) {
        DebugLog( D_ERROR, DC_ALL ) << "Not enought screen space...";
    }

    m_panel->SetSize( m_thisSize );

    switch( m_thisLocation ) {
        case Location::Centered:
            m_offset.x = ( TERMX > m_thisSize.x ) ? ( TERMX - m_thisSize.x ) / 2 : 0;
            m_offset.y = ( TERMY > m_thisSize.y ) ? ( TERMY - m_thisSize.y ) / 2 : 0;
            break;

        default:
            DebugLog( D_ERROR, DC_ALL ) << "Please state the location...";
            break;
    }

    // set up window
    m_wf_win = newwin( m_thisSize.y, m_thisSize.x, m_offset.y, m_offset.x );
    m_wf_winptr = WINDOW_PTR( m_wf_win );

    m_lastSize = m_thisSize;

    m_lastLocation = m_thisLocation;
    return 0;
}

void UIWindow::DrawEverything()
{
    werase( m_wf_win );
    m_panel->DrawEverything( m_wf_win, { 0, 0 } );
}

std::vector<std::shared_ptr<UIPanel>> UIPaddingPanel::GetChild() const
{
    return m_childPanels;
}

void UIPaddingPanel::AddChild( std::shared_ptr<UIPanel> panel )
{
    if( !m_childPanels.empty() ) {
        DebugLog( D_ERROR, DC_ALL ) << "Only supports one panel";
        return;
    }
    m_childPanels.push_back( panel );
}

void UIPaddingPanel::RemoveChild( size_t index )
{
    if( index != m_childPanels.size() - 1 ) {
        m_childPanels[index] = std::move( m_childPanels.back() );
    }
    m_childPanels.pop_back();
}

point UIPaddingPanel::RequestedSize( Sizes sizes )
{
    point size;

    if( m_drawBorder )
        size += { 2, 2 };

    if( m_childPanels.empty() ) {
        return size;
    }

    auto paneSize = m_childPanels[0]->RequestedSize( sizes );
    size += paneSize;

    return size;
}

// We are a simple border!
int UIPaddingPanel::SetSize( point size )
{
    m_thisSize = size;

    if( m_childPanels.empty() ) {
        return 0;
    }
    auto newSize = size;

    if( m_drawBorder )
        newSize -= { 2, 2 };

    m_childPanels[0]->SetSize( newSize );
    return 0;
}

void UIPaddingPanel::DrawEverything( WINDOW *wf_win, point offset )
{

    if( m_drawBorder ) {
        UIUtils::DrawBorder( wf_win, offset, m_thisSize );
    }

    if( !m_childPanels.empty() ) {
        point offset;

        if( m_drawBorder )
            offset += { 1, 1 };

        m_childPanels[0]->DrawEverything( wf_win, offset );
    }

    m_lastSize = m_thisSize;
}


UIPaddingPanel::UIPaddingPanel( bool drawBorder )
{
    m_drawBorder = drawBorder;
}

std::vector<std::shared_ptr<UIPanel>> UITabPanel::GetChild() const
{
    return m_childPanels;
}

void UITabPanel::AddChild( std::shared_ptr<UIPanel> panel )
{
    AddChild( std::to_string( m_childPanels.size() ), panel );
}

void UITabPanel::AddChild( std::string name, std::shared_ptr<UIPanel> panel )
{
    m_childPanels.push_back( panel );
    m_childPanelNames.push_back( name );
}

void UITabPanel::RemoveChild( size_t index )
{
    if( index != m_childPanels.size() - 1 ) {
        m_childPanels[index] = std::move( m_childPanels.back() );
    }
    m_childPanels.pop_back();

    if( index != m_childPanelNames.size() - 1 ) {
        m_childPanelNames[index] = std::move( m_childPanelNames.back() );
    }
    m_childPanelNames.pop_back();
}

point UITabPanel::RequestedSize( Sizes sizes )
{
    point size;

    size += { 0, 2 };

    if( m_drawBorder )
        size += { 2, 2 };

    if( m_childPanels.empty() ) {
        return size;
    }

    auto paneSize = m_childPanels[m_currentTab]->RequestedSize( sizes );
    size += paneSize;

    int len = 2;

    for( auto tx : m_childPanelNames ) {
        len += utf8_width( tx ) + 6;
    }

    if( len > size.x ) {
        size.x = len;
    }

    query_yn( std::to_string( size.x ).c_str() );
    query_yn( std::to_string( size.y ).c_str() );

    return size;
}

int UITabPanel::SetSize( point size )
{
    m_thisSize = size;

    if( m_childPanels.empty() ) {
        return 0;
    }
    auto newSize = size;

    if( m_drawBorder )
        newSize -= { 2, 2 };

    newSize -= { 0, 2 };

    m_childPanels[m_currentTab]->SetSize( newSize );
    return 0;
}

void UITabPanel::DrawEverything( WINDOW *wf_win, point offset )
{
    if( m_drawBorder ) {
        auto bOffset = offset;
        bOffset += { 0, 2 };

        auto bSize = m_thisSize;
        bSize -= { 0, 2 };

        UIUtils::DrawBorder( wf_win, bOffset, bSize );
    }

    int toffset = 2;
    for( size_t i = 0; i < m_childPanels.size(); i++ ) {
        toffset += 1;

        UIUtils::DrawTab( wf_win, offset, toffset, ( i == m_currentTab ), m_childPanelNames[i] );

        toffset += 5 + utf8_width( m_childPanelNames[i] );
    }

    if( !m_childPanels.empty() ) {
        point offset;

        if( m_drawBorder )
            offset += { 1, 1 };

        offset += { 0, 2 };

        m_childPanels[m_currentTab]->DrawEverything( wf_win, offset );
    }

    m_lastSize = m_thisSize;
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
