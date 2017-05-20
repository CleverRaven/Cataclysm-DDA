#include "UIHandler.h"

UIWindow::UIWindow(int minSizeX, int minSizeY, Location location)
{
    m_minSize.x = minSizeX;
    m_minSize.y = minSizeY;

    m_thisLocation = location;

    UpdateWindow();
}

int UIWindow::UpdateWindow()
{
    // TODO: Clear prev contents? Is that our job?

    m_thisSize = m_panel->PreferedRequestedSize();    
    
    IntPair minSize = m_panel->MinRequestedSize();

    if (m_minSize.x > m_thisSize.x) m_thisSize.x = m_minSize.x;
    if (m_minSize.y > m_thisSize.y) m_thisSize.y = m_minSize.y;

    if (m_thisSize.x > TERMX) m_thisSize.x = TERMX;
    if (m_thisSize.y > TERMY) m_thisSize.y = TERMY;

    if (m_minSize.x > TERMX) DebugLog(D_ERROR, DC_ALL) << "Not enought screen space...";
    if (m_minSize.y > TERMY) DebugLog(D_ERROR, DC_ALL) << "Not enought screen space...";

    if (minSize.x > TERMX) DebugLog(D_ERROR, DC_ALL) << "Not enought screen space...";
    if (minSize.y > TERMY) DebugLog(D_ERROR, DC_ALL) << "Not enought screen space...";
    
    m_panel->SetSize(m_thisSize);

    switch (m_thisLocation)
    {
    case Location::Centered:
    m_offset.x = (TERMX > m_thisSize.x) ? (TERMX - m_thisSize.x) / 2 : 0;
    m_offset.y = (TERMY > m_thisSize.y) ? (TERMY - m_thisSize.y) / 2 : 0;
    break;

    default:
    DebugLog(D_ERROR, DC_ALL) << "Please state the location...";
    break;
}

// set up window
m_wf_win = newwin(m_thisSize.y, m_thisSize.x, m_offset.y, m_offset.x);
m_wf_winptr = WINDOW_PTR( m_wf_win );

    m_lastSize = m_thisSize;

    m_lastLocation = m_thisLocation;
    return 0;
}

// UIPaddingPanel


// UIParentPanel
