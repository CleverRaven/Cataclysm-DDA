#pragma once
#ifndef UIHANDLER_H
#define UIHANDLER_H

#include "debug.h"

#include "cursesdef.h"
#include "output.h"

struct IntPair
{
    int x;
    int y;
};

class UIPanel
{
public:
    virtual std::vector<UIPanel*> getChild() = 0;
    virtual void addChild(UIPanel *panel) = 0;
    virtual void removeChild(size_t index) = 0; // Passes back removed child

    virtual IntPair RequestedSize(bool min) = 0;
    virtual int SetSize(IntPair size) = 0;
};
/*
// A generic panel to split crap
class UISplitPanel
{
public:
    enum class Arangments
    {
        Stacked,
        SideBySide,

        Undefined = 0,
        Total
    };

    UISplitPanel(Arangments arangment);

    std::vector<UIPanel*> getChild();
    void addChild(UIPanel *panel);

    // Passes back removed child
    // We swap this with the last then push back
    // Previous Indexes are not maintained!!!
    void removeChild(size_t index);

    IntPair RequestedSize(bool min);
    int SetSize(IntPair size);
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
    std::vector<UIPanel*> getChild();
    void addChild(UIPanel *panel);

    // Passes back removed child
    // We swap this with the last then push back
    // Previous Indexes are not maintained!!!
    void removeChild(size_t index);

    IntPair RequestedSize(bool min);
    int SetSize(IntPair size);
private:
    IntPair m_thisSize;
    IntPair m_lastSize;

    std::vector<UIPanel*> m_childPanels;
};

class UIWindow {
public:
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

    IntPair m_minSize;
    IntPair m_thisSize;
    IntPair m_lastSize;
    IntPair m_offset;

    WINDOW *m_wf_win = nullptr;
    WINDOW_PTR m_wf_winptr = nullptr;
};

#endif // UIHANDLER_H
