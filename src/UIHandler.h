#pragma once
#ifndef UIHANDLER_H
#define UIHANDLER_H

#include <vector>

#include "cursesdef.h"
#include "output.h"

#include "enums.h"
#include <memory>

namespace ui
{
enum class Sizes {
    Minimum,
    Prefered
};

namespace Utils
{
void DrawBorder( WINDOW *wf_win, point offset, point m_thisSize );
void DrawTab( WINDOW *wf_win, point offset, int tabOffset, bool tabActive,
              std::string text );
}

class Panel
{
    public:
        virtual point RequestedSize( Sizes sizes ) = 0;

        virtual void SetSize( point size ) = 0;

        virtual void DrawEverything( WINDOW *wf_window, point offset ) = 0;
};

class PaddingPanel : public Panel
{
    public:
        PaddingPanel( bool drawBorder );
        std::shared_ptr<Panel> GetChild() const;
        void SetChild( std::shared_ptr<Panel> panel );

        point RequestedSize( Sizes sizes ) override;
        void SetSize( point size ) override;

        void DrawEverything( WINDOW *wf_win, point offset ) override;
    private:
        point m_thisSize;

        std::shared_ptr<Panel> m_childPanel;
        bool m_drawBorder;
};

class TabPanel : public Panel
{
    public:
        TabPanel( bool drawBorder );

        std::vector<std::pair<std::string, std::shared_ptr<Panel>>> GetTabs() const;
        void AddTab( std::string name, std::shared_ptr<Panel> panel );
        void RemoveTab( size_t index );

        point RequestedSize( Sizes sizes ) override;
        void SetSize( point size ) override;

        void DrawEverything( WINDOW *wf_win, point offset ) override;

        void SwitchTab( size_t tab );
    private:
        size_t m_currentTab;

        point m_thisSize;

        // Could use std::map
        // But then GetChild function would get way more complex
        // I think this is easier
        std::vector<std::pair<std::string, std::shared_ptr<Panel>>> m_childPanels;

        bool m_drawBorder;
};

class Window
{
    public:
        // TODO REMOVE EVENTUALY!!!
        WINDOW *LegacyWindow() {
            return m_wf_win;
        };

        enum class Location {
            Centered
        };

        Window( int minSizeX, int minSizeY, Location location, bool drawBorder );
        void UpdateWindowSize();

        void DrawEverything();

        std::unique_ptr<PaddingPanel> m_panel;
    private:
        Location m_thisLocation;

        point m_minSize;
        point m_thisSize;
        point m_offset;

        WINDOW *m_wf_win;
        WINDOW_PTR m_wf_winptr;
};
}
#endif // UIHANDLER_H
