#include "UIHandler.h"

UIWindow::UIWindow(int minSizeX, int minSizeY, Location location, bool drawBorder) : m_panel(new UIParentPanel(drawBorder))
{
    m_minSize.x = minSizeX;
    m_minSize.y = minSizeY;

    m_thisLocation = location;

    UpdateWindow();
}

int UIWindow::UpdateWindow()
{
    // TODO: Clear prev contents? Is that our job?

    m_thisSize = m_panel->RequestedSize(Sizes::Prefered);    
    
    point minSize = m_panel->RequestedSize(Sizes::Minimum);

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

void UIWindow::DrawEverything()
{
    m_panel->DrawEverything(m_wf_win, { 0, 0 });
}

/*
UISplitPanel::UISplitPanel(Arangments arangment)
{
    m_arangment = arangment;
}

std::vector<std::shared_ptr<UIPanel>> UISplitPanel::getChild() const
{
    return m_childPanels; 
}

void UISplitPanel::addChild(std::shared_ptr<UIPanel> panel)
{
    m_childPanels.push_back(panel);
}

void UISplitPanel::removeChild(size_t index)
{
    if (index != m_childPanels.size() -1)
        m_childPanels[index] = std::move(m_childPanels.back());
    m_childPanels.pop_back();
}

point UISplitPanel::RequestedSize(Sizes sizes)
{
    point size = { 2, 2 };

    switch (m_arangement)
    {
    case Arangments::Stacked:
        // |-|
        // |A| minX = smallest x + 2
        // |-|
        // |B| minY = Sum of Ys + # children - 1 + 2
        // |-|
        // |C|
        // |-|
        // |D|
        // |-|
        // |E|
        // |-|
        size.y += m_childPanels.size() - 1;
        
        for (auto panel : m_childPanels)
        {
            auto reqSize = panel->RequestedSize(min);

            if (reqSize.x > size.x) size.x = reqSize.x;

            size.y += reqSize.y;
        }
        break;
    case Arangments::SideBySide:
        // |-+-+-+-+-| minX = Sum of Xs + # children - 1 + 2
        // |A|B|C|D|E| minY = smallest Y + 2
        // |-+-+-+-+-|
        size.x += m_childPanels.size() - 1;
        
        for (auto panel : m_childPanels)
        {
            auto reqSize = panel->RequestedSize(min);

            if (reqSize.y > size.y) size.y = reqSize.y;

            size.x += reqSize.x;
        }
        break;
    }
}
int UISplitPanel::SetSize(point size);
*/
// UIParentPanel
std::vector<std::shared_ptr<UIPanel>> UIParentPanel::getChild() const
{
    return m_childPanels; 
}

void UIParentPanel::addChild(std::shared_ptr<UIPanel> panel)
{   if (m_childPanels.empty())
    {
        DebugLog(D_ERROR, DC_ALL) << "Only supports one panel";
        return;
    }
    m_childPanels.push_back(panel);
}

void UIParentPanel::removeChild(size_t index)
{
    if (index != m_childPanels.size() -1)
        m_childPanels[index] = std::move(m_childPanels.back());
    m_childPanels.pop_back();
}

point UIParentPanel::RequestedSize(Sizes sizes)
{
    point size = { 2, 2 };

    if (m_childPanels.empty())
        return size;

    auto paneSize = m_childPanels[0]->RequestedSize(sizes);
    size += paneSize;

    return size;
}

// We are a simple border!
int UIParentPanel::SetSize(point size)
{
    m_thisSize = size;

    if (m_childPanels.empty())
        return 0;
    auto newSize = size;
    newSize -= { 2, 2 };
    m_childPanels[0]->SetSize(newSize);
    return 0;
}

void UIParentPanel::DrawEverything(WINDOW *wf_win, point offset)
{
    werase(wf_win);
    
    if (m_drawBorder)
        UIUtils::DrawBorder(wf_win, offset, m_thisSize);

    if (!m_childPanels.empty())
    {
        m_childPanels[0]->DrawEverything(wf_win, offset += { 1, 1 });
    }

    m_lastSize = m_thisSize;
}


void UIUtils::DrawBorder(WINDOW *wf_win, point offset, point m_thisSize)
{
    // Bottom and top border
    for (int i = 1; i < m_thisSize.x - 1; i++) 
    {
        mvwputch(wf_win, offset.y                   , i + offset.x, BORDER_COLOR, LINE_OXOX);
        mvwputch(wf_win, offset.y + m_thisSize.y - 1, i + offset.x, BORDER_COLOR, LINE_OXOX);
    }

    // Right and left border
    for (int i = 1; i < m_thisSize.y - 1; i++) 
    {
        mvwputch(wf_win, i + offset.y, offset.x                    , BORDER_COLOR, LINE_XOXO);
        mvwputch(wf_win, i + offset.y, offset.x + m_thisSize.x - 1 , BORDER_COLOR, LINE_XOXO);
    }
        
    // Corners
    mvwputch(wf_win, offset.y                    , offset.x                      , BORDER_COLOR, LINE_OXXO); // |^
    mvwputch(wf_win, offset.y                    , offset.x + m_thisSize.x - 1   , BORDER_COLOR, LINE_OOXX); // ^|
    mvwputch(wf_win, offset.y + m_thisSize.y - 1 , offset.x                      , BORDER_COLOR, LINE_XXOO); // |_
    mvwputch(wf_win, offset.y + m_thisSize.y - 1 , offset.x + m_thisSize.x - 1   , BORDER_COLOR, LINE_XOOX); // _|
}

UIParentPanel::UIParentPanel(bool drawBorder)
{
    m_drawBorder = drawBorder;
}
