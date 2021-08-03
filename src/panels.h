#pragma once
#ifndef CATA_SRC_PANELS_H
#define CATA_SRC_PANELS_H

#include <functional>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "coordinates.h"
#include "translations.h"

class JsonIn;
class JsonOut;
class avatar;
struct point;

namespace activity_level
{
std::string activity_level_str( float level );
} // namespace activity_level

namespace catacurses
{
class window;
} // namespace catacurses
enum face_type : int {
    face_human = 0,
    face_bird,
    face_bear,
    face_cat,
    num_face_types
};

namespace overmap_ui
{
void draw_overmap_chunk( const catacurses::window &w_minimap, const avatar &you,
                         const tripoint_abs_omt &global_omt, const point &start, int width,
                         int height );
} // namespace overmap_ui

bool default_render();

class window_panel
{
    public:
        window_panel( const std::function<void( avatar &, const catacurses::window & )> &draw_func,
                      const std::string &id, const translation &nm, int ht, int wd, bool default_toggle_,
                      const std::function<bool()> &render_func = default_render, bool force_draw = false );

        std::function<void( avatar &, const catacurses::window & )> draw;
        std::function<bool()> render;

        int get_height() const;
        int get_width() const;
        const std::string &get_id() const;
        std::string get_name() const;
        bool toggle;
        bool always_draw;

    private:
        int height;
        int width;
        std::string id;
        translation name;
};

class panel_layout
{
    public:
        panel_layout( const translation &_name,
                      const std::vector<window_panel> &_panels );

        std::string name() const;
        const std::vector<window_panel> &panels() const;
        std::vector<window_panel> &panels();
    private:
        translation _name;
        std::vector<window_panel> _panels;
};

class panel_manager
{
    public:
        panel_manager();
        ~panel_manager() = default;
        panel_manager( panel_manager && ) = default;
        panel_manager( const panel_manager & ) = default;
        panel_manager &operator=( panel_manager && ) = default;
        panel_manager &operator=( const panel_manager & ) = default;

        static panel_manager &get_manager() {
            static panel_manager single_instance;
            return single_instance;
        }

        panel_layout &get_current_layout();
        std::string get_current_layout_id() const;
        int get_width_right();
        int get_width_left();

        void show_adm();

        void init();

    private:
        bool save();
        bool load();
        void serialize( JsonOut &json );
        void deserialize( JsonIn &jsin );
        // update the screen offsets so the game knows how to adjust the main window
        void update_offsets( int x );

        // The amount of screen space from each edge the sidebar takes up
        int width_right = 0;
        int width_left = 0;
        std::string current_layout_id;
        std::map<std::string, panel_layout> layouts;

};

#endif // CATA_SRC_PANELS_H
