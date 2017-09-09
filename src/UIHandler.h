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
void draw_border( WINDOW *wf_win, point offset, point this_size );

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
 * \param tab_offset Additional offset on the x axis
 * \param tab_active Whether or not the tab is currently active
 * \param text Tab's name
 */
void draw_tab( WINDOW *wf_win, point offset, int tab_offset, bool tab_active,
              std::string text );
}

class panel
{
    public:
        /**
         * Returns either its prefered size or its minimum size
         *
         * \param size Whether you want the prefered size or minimum size
         */
        virtual point requested_size( sizes size ) = 0;

        /**
         * Sets the size
         *
         * \param size Size to set size to
         *
         * \remarks Will crash program if size is less than the minimum size.
         */
        virtual void set_size( point size ) = 0;

        /**
         * Draws this panel and then tells children to draw themselfs
         *
         * \param wf_win Window to draw content in
         * \param offset where to start drawing
         */
        virtual void draw_everything( WINDOW *wf_win, point offset ) = 0;
};

class window;

class padding_panel : public panel
{
    public:
        padding_panel( bool new_draw_border, ui::window* parent_win );
        std::shared_ptr<panel> get_child() const;
        void set_child( std::shared_ptr<panel> new_child_panel );

        point requested_size( sizes size ) override;
        void set_size( point size ) override;
        void draw_everything( WINDOW *wf_win, point offset ) override;
    private:
        ui::window* parent_win;

        point this_size;

        std::shared_ptr<panel> child_panel;
        bool draw_border;
};

class tab_panel : public panel
{
    public:
        tab_panel( bool new_draw_border, ui::window* parent_win );

        std::vector<std::pair<std::string, std::shared_ptr<panel>>> get_tabs() const;
        void add_tab( std::string name, std::shared_ptr<panel> tab_panel );
        void remove_tab( size_t index );

        point requested_size( sizes size ) override;
        void set_size( point size ) override;

        void draw_everything( WINDOW *wf_win, point offset ) override;

        void switch_tab( size_t tab );
    private:
        ui::window* parent_win;

        size_t current_tab = 0;

        point this_size;

        std::vector<std::pair<std::string, std::shared_ptr<panel>>> child_panels;

        bool draw_border;
};

class window
{
    public:
        // TODO REMOVE EVENTUALY!!!
        WINDOW *legacy_window() {
            return wf_win;
        };

        enum class location {
            CENTERED
        };

        window( int min_size_x, int min_size_y, location new_location, bool new_draw_border );
        void update_window_size();

        void draw_everything();

        std::unique_ptr<padding_panel> main_panel;
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
