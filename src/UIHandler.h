#pragma once
#ifndef UIHANDLER_H
#define UIHANDLER_H

#include "debug.h"

#include "cursesdef.h"
#include "output.h"

#include "enums.h"
#include <memory>

enum class Sizes
{
    Minimum,
    Prefered
};

class UIUtils 
{
public:
    static void DrawBorder(WINDOW *wf_win, point offset, point m_thisSize);
};

class UIPanel
{
public:
    virtual std::vector<std::shared_ptr<UIPanel>> getChild() const = 0;
    virtual void addChild(std::shared_ptr<UIPanel> panel) = 0;
    virtual void removeChild(size_t index) = 0;

    virtual point RequestedSize(Sizes sizes) = 0;
    virtual int SetSize(point size) = 0;

    virtual void DrawEverything(WINDOW *wf_window, point offset) = 0;
};
/*
// A generic panel to split crap
class UISplitPanel
{
public:
    enum class Arangments
    {
        SideBySide,
        Stacked,

        Undefined = 0,
        Total
    };

    UISplitPanel(Arangments arangment);

    std::vector<std::shared_ptr<UIPanel>> getChild() = 0;
    void addChild(std::shared_ptr<UIPanel> panel);

    // Passes back removed child
    // We swap this with the last then push back
    // Previous Indexes are not maintained!!!
    void removeChild(size_t index);

    point RequestedSize(Sizes sizes);
    int SetSize(point size);
private:
    std::vector<std::shared_ptr<UIPanel>> m_childPanels;
    Arangments m_arangment;
};
*/
// to be used _ONLY_ in windows
// If you want a generic panel use PaddingPanel
class UIParentPanel : public UIPanel
{
public:
    UIParentPanel(bool drawBorder);
    std::vector<std::shared_ptr<UIPanel>> getChild() const;
    void addChild(std::shared_ptr<UIPanel> panel);

    // Passes back removed child
    // We swap this with the last then push back
    // Previous Indexes are not maintained!!!
    void removeChild(size_t index);

    point RequestedSize(Sizes sizes);
    int SetSize(point size);

    void DrawEverything(WINDOW *wf_win, point offset);
private:
    point m_thisSize;
    point m_lastSize;

    std::vector<std::shared_ptr<UIPanel>> m_childPanels;
    bool m_drawBorder;
};

class UIWindow {
public:
    // TODO REMOVE EVENTUALY!!!
    WINDOW* LegacyWindow() {return m_wf_win;};


    enum class Location
    {
        Centered,

        Undefined = 0,
        Total
    };

    UIWindow(int minSizeX, int minSizeY, Location location, bool drawBorder);
    int UpdateWindow();

    void DrawEverything();
private:
    std::unique_ptr<UIPanel> m_panel;
    
    Location m_thisLocation;
    Location m_lastLocation;

    point m_minSize;
    point m_thisSize;
    point m_lastSize;
    point m_offset;

    WINDOW *m_wf_win = nullptr;
    WINDOW_PTR m_wf_winptr = nullptr;
};

#endif // UIHANDLER_H
