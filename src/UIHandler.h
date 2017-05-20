#pragma once
#ifndef UIHANDLER_H
#define UIHANDLER_H

#include "debug.h"

#include "cursesdef.h"
#include "output.h"

#include "enums.h"

class UIPanel
{
public:
    virtual std::vector<UIPanel*> getChild() const = 0;
    virtual void addChild(UIPanel *panel) = 0;
    virtual void removeChild(size_t index) = 0;

    virtual point RequestedSize(bool min) = 0;
    virtual int SetSize(point size) = 0;
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

    std::vector<UIPanel*> getChild() = 0;
    void addChild(UIPanel *panel);

    // Passes back removed child
    // We swap this with the last then push back
    // Previous Indexes are not maintained!!!
    void removeChild(size_t index);

    point RequestedSize(bool min);
    int SetSize(point size);
private:
    std::vector<UIPanel*> m_childPanels;
    Arangments m_arangment;
};
*/
// to be used _ONLY_ in windows
// If you want a generic panel use PaddingPanel
class UIParentPanel : public UIPanel
{
public:
    std::vector<UIPanel*> getChild() const;
    void addChild(UIPanel *panel);

    // Passes back removed child
    // We swap this with the last then push back
    // Previous Indexes are not maintained!!!
    void removeChild(size_t index);

    point RequestedSize(bool min);
    int SetSize(point size);
private:
    point m_thisSize;
    //point m_lastSize;

    std::vector<UIPanel*> m_childPanels;
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

    UIWindow(int minSizeX, int minSizeY, Location location);
    int UpdateWindow();
private:
    UIPanel* m_panel = new UIParentPanel();
    
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
