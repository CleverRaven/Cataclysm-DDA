#pragma once
#ifndef PANELS_H
#define PANELS_H

#include "json.h"

#include <functional>
#include <map>
#include <string>

class player;
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

class window_panel
{
    public:
        window_panel( std::function<void( player &, const catacurses::window & )> draw_func, std::string nm,
                      int ht, int wd, bool default_toggle );

        std::function<void( player &, const catacurses::window & )> draw;
        int get_height() const;
        int get_width() const;
        std::string get_name() const;
        bool toggle;

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
        const std::string get_current_layout_id() const;

        void draw_adm( const catacurses::window &w, size_t column = 0, size_t index = 1 );

        void init();

    private:
        bool save();
        bool load();
        void serialize( JsonOut &json );
        void deserialize( JsonIn &jsin );
        // update the screen offsets so the game knows how to adjust the main window
        void update_offsets( int x );

        std::string current_layout_id;
        std::map<std::string, std::vector<window_panel>> layouts;

};

#endif //PANELS_H
