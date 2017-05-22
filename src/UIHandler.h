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
/**
 * Draws a border.
 *
 * <pre>
 *  ____ m_thisSize.x
 * /    \
 * v    v
 * +----+<-\
 * |    |  | m_thisSize.y
 * |    |  |
 * +----+<-/
 *
 * + - - - - - - - - - +
 *            ^
 * |          |offset.y|
 *            v
 * |          +----+   |
 *   offset.x |    |
 * |<-------->+----+   | <- wf_win
 *
 * + - - - - - - - - - +
 * </pre>
 *
 * \param wf_win The window to draw the border in
 * \param offset Where to start the border
 * \param m_thisSize The size of the area to be covered.
 */
void DrawBorder( WINDOW *wf_win, point offset, point m_thisSize );

/**
 * Draws tabs
 *
 * <pre>
 * + - - - - - - - - - +
 *   offset.x            <-\
 * |     +             |   |
 *   tabOffset             |
 * |/--------\         |   | offset.y
 *  V        V             |
 * |  /----\   /---\   | <-/
 *   <|text|>  |tx2| <- Inactive tab
 * |--/    \\-----------|
 *      ^Active Tab
 * + - - - - - - - - - + <- wf_win
 * </pre>
 *
 * \param wf_win The window to draw the tab in
 * \param offset Where to start the tab
 * \param tabOffset Additional offset on the x axis
 * \param tabActive Whether or not the tab is currently active
 * \param text Tab's name
 */
void DrawTab( WINDOW *wf_win, point offset, int tabOffset, bool tabActive,
              std::string text );
}

class Panel
{
    public:
        /**
         * Returns either its Prefered size or its Minimum size
         *
         * \param sizes Whether you want the prefered size or minimum size
         */
        virtual point RequestedSize( Sizes sizes ) = 0;

        /**
         * Sets the size
         *
         * \param size Size to set size to
         *
         * \remarks Will crash program if size is less than the minimum size.
         */
        virtual void SetSize( point size ) = 0;

        /**
         * Draws this panel and then tells children to draw themselfs
         *
         * \param wf_win Window to draw content in
         * \param offset where to start drawing
         */
        virtual void DrawEverything( WINDOW *wf_win, point offset ) = 0;
};

class PaddingPanel : public Panel
{
    public:
        PaddingPanel( bool newDrawBorder );
        std::shared_ptr<Panel> GetChild() const;
        void SetChild( std::shared_ptr<Panel> panel );

        point RequestedSize( Sizes sizes ) override;
        void SetSize( point size ) override;
        void DrawEverything( WINDOW *wf_win, point offset ) override;
    private:
        point thisSize;

        std::shared_ptr<Panel> childPanel;
        bool drawBorder;
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
        size_t m_currentTab = 0;

        point m_thisSize;

        std::vector<std::pair<std::string, std::shared_ptr<Panel>>> m_childPanels;

        bool m_drawBorder;
};

class Window
{
    public:
        // TODO REMOVE EVENTUALY!!!
        WINDOW *LegacyWindow() {
            return wf_win;
        };

        enum class Location {
            Centered
        };

        Window( int minSizeX, int minSizeY, Location location, bool newDrawBorder );
        void UpdateWindowSize();

        void DrawEverything();

        std::unique_ptr<PaddingPanel> panel;
    private:
        Location m_thisLocation;

        point minSize;
        point thisSize;
        point offset;

        WINDOW *wf_win;
        WINDOW_PTR wf_winptr;
};
}
#endif // UIHANDLER_H
