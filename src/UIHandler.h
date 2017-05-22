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
enum class sizes {
    MINIMUM,
    PREFERED
};

namespace utils
{
/**
 * Draws a border.
 *
 * <pre>
 *  ____ this_size.x
 * /    \
 * v    v
 * +----+<-\
 * |    |  | this_size.y
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
 * \param this_size The size of the area to be covered.
 */
void DrawBorder( WINDOW *wf_win, point offset, point this_size );

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
void DrawTab( WINDOW *wf_win, point offset, int tab_offset, bool tab_active,
              std::string text );
}

class Panel
{
    public:
        /**
         * Returns either its prefered size or its minimum size
         *
         * \param size Whether you want the prefered size or minimum size
         */
        virtual point RequestedSize( sizes size ) = 0;

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
        PaddingPanel( bool new_draw_border );
        std::shared_ptr<Panel> GetChild() const;
        void SetChild( std::shared_ptr<Panel> new_child_panel );

        point RequestedSize( sizes size ) override;
        void SetSize( point size ) override;
        void DrawEverything( WINDOW *wf_win, point offset ) override;
    private:
        point this_size;

        std::shared_ptr<Panel> child_panel;
        bool draw_border;
};

class TabPanel : public Panel
{
    public:
        TabPanel( bool new_draw_border );

        std::vector<std::pair<std::string, std::shared_ptr<Panel>>> GetTabs() const;
        void AddTab( std::string name, std::shared_ptr<Panel> tab_panel );
        void RemoveTab( size_t index );

        point RequestedSize( sizes size ) override;
        void SetSize( point size ) override;

        void DrawEverything( WINDOW *wf_win, point offset ) override;

        void SwitchTab( size_t tab );
    private:
        size_t current_tab = 0;

        point this_size;

        std::vector<std::pair<std::string, std::shared_ptr<Panel>>> child_panels;

        bool draw_border;
};

class window
{
    public:
        // TODO REMOVE EVENTUALY!!!
        WINDOW *LegacyWindow() {
            return wf_win;
        };

        enum class location {
            CENTERED
        };

        window( int min_size_x, int min_size_y, location new_location, bool new_draw_border );
        void UpdateWindowSize();

        void DrawEverything();

        std::unique_ptr<PaddingPanel> main_panel;
    private:
        location this_location;

        point min_size;
        point this_size;
        point offset;

        WINDOW *wf_win;
        WINDOW_PTR wf_winptr;
};
}
#endif // UIHANDLER_H
