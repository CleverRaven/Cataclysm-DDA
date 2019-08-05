#pragma once
#ifndef PANELS_H
#define PANELS_H

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

class avatar;
class JsonIn;
class JsonOut;

struct tripoint;

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
                         const tripoint &global_omt, const int start_y, const int start_x, const int width,
                         const int height );
} // namespace overmap_ui

bool default_render();

class window_panel
{
    public:
        window_panel( std::function<void( avatar &, const catacurses::window & )> draw_func,
                      const std::string &nm, int ht, int wd, bool default_toggle,
                      std::function<bool()> render_func = default_render, bool force_draw = false );

        std::function<void( avatar &, const catacurses::window & )> draw;
        std::function<bool()> render;

        int get_height() const;
        int get_width() const;
        std::string get_name() const;
        bool toggle;
        bool always_draw;

    private:
        int height;
        int width;
        bool default_toggle;
        std::string name;
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

        std::vector<window_panel> &get_current_layout();
        std::string get_current_layout_id() const;
        int get_width_right();
        int get_width_left();

        void draw_adm( const catacurses::window &w, size_t column = 0, size_t index = 1 );

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
        std::map<std::string, std::vector<window_panel>> layouts;

};

#endif //PANELS_H
