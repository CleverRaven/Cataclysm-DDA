#pragma once
#ifndef CATA_SRC_PANELS_H
#define CATA_SRC_PANELS_H

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "coords_fwd.h"
#include "translation.h"
#include "type_id.h"

class JsonArray;
class JsonOut;
class avatar;
class widget;
struct point;

enum face_type : int {
    face_human = 0,
    face_bird,
    face_bear,
    face_cat,
    num_face_types
};

namespace catacurses
{
class window;
} // namespace catacurses

namespace overmap_ui
{
void draw_overmap_chunk( const catacurses::window &w_minimap, const avatar &you,
                         const tripoint_abs_omt &global_omt, const point &start, int width,
                         int height );
void draw_overmap_chunk_imgui( const avatar &you, const tripoint_abs_omt &global_omt,
                               int width, int height );
} // namespace overmap_ui

bool default_render();

// Arguments to pass into the static draw function (in window_panel::draw)
// Includes public avatar (_ava) and window (_win) references, and private widget reference
// passed to the constructor, accessible with get_widget().
struct draw_args {
    public:
        const avatar &_ava;
        const catacurses::window &_win;

        draw_args( const avatar &a, const catacurses::window &w, const widget_id &wgt ) :
            _ava( a ), _win( w ), _wgt( wgt ) {}

        widget *get_widget() const {
            return _wgt.is_null() ? nullptr : const_cast<widget *>( &*_wgt );
        }
    private:
        widget_id _wgt;
};

// A window_panel is a rectangular region or drawable area within the sidebar window.
// It corresponds to a section that the player may toggle or rearrange from the in-game sidebar options.
// It is associated with a draw function (taking draw_args with avatar and window), along with id and name.
// The height, width, and default toggle state must be provided to the constructor as well.
class window_panel
{
    public:
        window_panel( const std::function<void( const draw_args & )> &draw_func,
                      const std::string &id, const translation &nm, int ht, int wd, bool default_toggle_,
                      const std::function<bool()> &render_func = default_render, bool force_draw = false );

        window_panel( const std::string &id, const translation &nm, int ht, int wd, bool default_toggle_,
                      const std::function<bool()> &render_func = default_render, bool force_draw = false );

        // Draw function returns the diff in height on the sidebar.
        // This is to reclaim any lines from skipped widgets.
        std::function<int( const draw_args & )> draw;
        std::function<bool()> render;

        int get_height() const;
        int get_width() const;
        const std::string &get_id() const;
        std::string get_name() const;
        void set_widget( const widget_id &w );
        const widget_id &get_widget() const;
        void set_draw_func( const std::function<int( const draw_args & )> &draw_func );
        bool toggle;
        bool always_draw;

    private:
        int height;
        int width;
        widget_id wgt;
        std::string id;
        translation name;
};

// A panel_layout is a collection of window_panels drawn in order from top to bottom.
// It is associated with the user-selectable layouts named "classic", "compact", "labels", etc.
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

// The panel_manager allows the player choose their current panel layout and window panels.
// The player's selected panel_layout, enabled window_panels and what order they appear in,
// are saved to the PATH_INFO::panel_options() file, typically config/panel_options.json.
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
        widget *get_current_sidebar();
        widget *get_sidebar( const std::string &name );
        std::string get_current_layout_id() const;
        int get_width_right() const;
        int get_width_left() const;

        void show_adm();

        void init();

    private:
        bool save();
        bool load();
        void serialize( JsonOut &json );
        void deserialize( const JsonArray &ja );
        // update the screen offsets so the game knows how to adjust the main window
        void update_offsets( int x );

        // The amount of screen space from each edge the sidebar takes up
        int width_right = 0; // NOLINT(cata-serialize)
        int width_left = 0; // NOLINT(cata-serialize)
        std::string current_layout_id;
        std::map<std::string, panel_layout> layouts;

        friend widget;
};

#endif // CATA_SRC_PANELS_H
