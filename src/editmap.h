#pragma once
#ifndef EDITMAP_H
#define EDITMAP_H

#include <functional>
#include <map>
#include <vector>
#include <string>

#include "optional.h"
#include "color.h"
#include "cursesdef.h"
#include "point.h"
#include "type_id.h"

struct real_coords;
class Creature;
class field;
class uilist;
class vehicle;
class map;
class tinymap;

enum shapetype {
    editmap_rect, editmap_rect_filled, editmap_line, editmap_circle,
};

class editmap;

struct editmap_hilight {
    std::vector<bool> blink_interval;
    int cur_blink;
    nc_color color;
    std::map<tripoint, char> points;
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
        tripoint pos2screen( const tripoint &p );
        bool eget_direction( tripoint &p, const std::string &action ) const;
        cata::optional<tripoint> edit();
        void uber_draw_ter( const catacurses::window &w, map *m );
        void update_view_with_help( const std::string &txt, const std::string &title );

        // T_t can be ter_t, furn_t, and trap
        template<typename T_t>
        void edit_feature();
        void edit_fld();
        void edit_itm();
        void edit_critter( Creature &critter );
        void edit_veh();
        void edit_mapgen();
        void cleartmpmap( tinymap &tmpmap );
        void mapgen_preview( const real_coords &tc, uilist &gmenu );
        vehicle *mapgen_veh_query( const tripoint &omt_tgt );
        bool mapgen_veh_destroy( const tripoint &omt_tgt, vehicle *car_target );
        void mapgen_retarget();
        int select_shape( shapetype shape, int mode = -1 );

        void update_fmenu_entry( uilist &fmenu, field &field, field_type_id idx );
        void setup_fmenu( uilist &fmenu );
        catacurses::window w_info;
        int width;
        int height;
        int offsetX;
        int infoHeight;

        void recalc_target( shapetype shape );
        bool move_target( const std::string &action, int moveorigin = -1 );

        int sel_field;
        int sel_field_intensity;

        tripoint target;
        tripoint origin;
        bool moveall;
        bool refresh_mplans;
        shapetype editshape;

        std::vector<tripoint> target_list;
        cata::optional<std::function<void( const tripoint &p )>> draw_target_override;
        std::map<std::string, editmap_hilight> hilights;
        bool blink;
        bool altblink;
        point tmax;
        bool uberdraw;

        editmap();
        ~editmap();
};

#endif
