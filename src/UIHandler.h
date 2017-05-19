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
    virtual int addChild(UIPanel *panel) = 0;
    virtual UIPanel* removeChild(int index0 = 0; // Passes back removed child

    virtual IntPair MinRequestedSize() = 0;
    virtual IntPair PerferedRequestedSize() = 0;
    virtual int SetSize() = 0;
};

// A generic panel to hold crap
class UIPaddingPanel
{
};

// to be used _ONLY_ in windows
// If you want a generic panel use PaddingPanel
class UIParentPanel : public UIPanel
{
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
