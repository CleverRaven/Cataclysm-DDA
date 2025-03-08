#pragma once
#ifndef CATA_SRC_EDITMAP_H
#define CATA_SRC_EDITMAP_H

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "color.h"
#include "coordinates.h"
#include "cursesdef.h"
#include "memory_fast.h"
#include "point.h"
#include "type_id.h"

class Creature;
class field;
class input_context;
class map;
class smallmap;
class tinymap;
class ui_adaptor;
class uilist;
class vehicle;

enum shapetype {
    editmap_rect, editmap_rect_filled, editmap_line, editmap_circle,
};

class editmap;

struct editmap_hilight {
    std::vector<bool> blink_interval;
    int cur_blink = 0;
    nc_color color;
    std::map<tripoint_bub_ms, char> points;
    nc_color( *getbg )( const nc_color & );
    void setup() {
        getbg = color == c_red ? &red_background :
                color == c_magenta ? &magenta_background :
                color == c_cyan ? &cyan_background :
                color == c_yellow ? &yellow_background : &green_background;
    }
    void draw( editmap &em, bool update = false );
};

class editmap
{
    public:
        tripoint pos2screen( const tripoint_bub_ms &p );
        bool eget_direction( tripoint_rel_ms &p, const std::string &action,
                             const input_context &ctxt ) const;
        std::optional<tripoint_bub_ms> edit();
        void uber_draw_ter( const catacurses::window &w, map *m );
        void update_view_with_help( const std::string &txt, const std::string &title );

        // T_t can be ter_t, furn_t, and trap
        template<typename T_t>
        void edit_feature();
        void edit_fld();
        void edit_rads() const;
        void edit_itm();
        void edit_critter( Creature &critter );
        void edit_mapgen();
        void cleartmpmap( smallmap &tmpmap ) const;
        void mapgen_preview( const real_coords &tc, uilist &gmenu );
        vehicle *mapgen_veh_query( const tripoint_abs_omt &omt_tgt );
        bool mapgen_veh_destroy( const tripoint_abs_omt &omt_tgt, vehicle *car_target );
        void mapgen_retarget();
        int select_shape( shapetype shape, int mode = -1 );

        void update_fmenu_entry( uilist &fmenu, field &field, const field_type_id &idx );
        void setup_fmenu( uilist &fmenu );
        catacurses::window w_info;

        void recalc_target( shapetype shape );
        bool move_target( const std::string &action, const input_context &ctxt, int moveorigin = -1 );

        int sel_field;
        int sel_field_intensity;

        tripoint_bub_ms target;
        tripoint_bub_ms origin;
        bool moveall;
        bool refresh_mplans;
        shapetype editshape;

        std::vector<tripoint_bub_ms> target_list;
        std::function<void( const tripoint_bub_ms &p )> draw_target_override;
        std::map<std::string, editmap_hilight> hilights;
        bool blink;
        bool altblink;
        bool uberdraw;

        editmap();
        ~editmap();

    private:
        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();

        weak_ptr_fast<ui_adaptor> ui;

        std::string info_txt_curr;
        std::string info_title_curr;

        tinymap *tmpmap_ptr = nullptr;

        const int width = 60;
        const int offsetX = 0;
        const int infoHeight = 20;

        point tmax;

        bool run_post_process = true;

        void draw_main_ui_overlay();
        void do_ui_invalidation();

        // work around the limitation that you can't forward declare an inner class
        class game_draw_callback_t_container;

        std::unique_ptr<game_draw_callback_t_container> draw_cb_container_;
        game_draw_callback_t_container &draw_cb_container();
};

#endif // CATA_SRC_EDITMAP_H
