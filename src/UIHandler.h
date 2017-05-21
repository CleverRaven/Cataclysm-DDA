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
    static void DrawTab(WINDOW *wf_win, point offset, int tabOffset, bool tabActive, std::string text);
};

class UIPanel
{
public:
    virtual std::vector<std::shared_ptr<UIPanel>> GetChild() const = 0;
    virtual void AddChild(std::shared_ptr<UIPanel> panel) = 0;
    virtual void RemoveChild(size_t index) = 0;

    virtual point RequestedSize(Sizes sizes) = 0;
    virtual int SetSize(point size) = 0;

    virtual void DrawEverything(WINDOW *wf_window, point offset) = 0;
};

class UIPaddingPanel : public UIPanel
{
public:
    UIPaddingPanel(bool drawBorder);
    std::vector<std::shared_ptr<UIPanel>> GetChild() const;
    void AddChild(std::shared_ptr<UIPanel> panel);

    // We swap this with the last then push back
    // Previous Indexes are not maintained!!!
    void RemoveChild(size_t index);

    point RequestedSize(Sizes sizes);
    int SetSize(point size);

    void DrawEverything(WINDOW *wf_win, point offset);
private:
    point m_thisSize;
    point m_lastSize;

    std::vector<std::shared_ptr<UIPanel>> m_childPanels;
    bool m_drawBorder;
};

class UITabPanel : public UIPanel
{
public:
    UITabPanel(bool drawBorder);
    std::vector<std::shared_ptr<UIPanel>> GetChild() const;
    
    //Please don't, use one below and give it a proper name. 
    void AddChild(std::shared_ptr<UIPanel> panel);
    void AddChild(std::string name, std::shared_ptr<UIPanel> panel);

    // We swap this with the last then push back
    // Previous Indexes are not maintained!!!
    void RemoveChild(size_t index);

    point RequestedSize(Sizes sizes);
    int SetSize(point size);

    void DrawEverything(WINDOW *wf_win, point offset);

    void SwitchTab(size_t tab);
private:
    size_t m_currentTab;

    point m_thisSize;
    point m_lastSize;

    // Could use std::map
    // But then GetChild function would get way more complex
    // I think this is easier
    std::vector<std::shared_ptr<UIPanel>> m_childPanels;
    std::vector<std::string> m_childPanelNames;

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
    
    std::unique_ptr<UIPanel> m_panel;
private:
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
