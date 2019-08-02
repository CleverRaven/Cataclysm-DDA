#pragma once
#ifndef EDITMAP_H
#define EDITMAP_H

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
        void uphelp( const std::string &txt1 = "", const std::string &txt2 = "",
                     const std::string &title = "" );
        tripoint pos2screen( const tripoint &p );
        tripoint screen2pos( const tripoint &p );
        bool eget_direction( tripoint &p, const std::string &action ) const;
        cata::optional<tripoint> edit();
        void uber_draw_ter( const catacurses::window &w, map *m );
        void update_view( bool update_info = false );
        int edit_ter();

        int edit_fld();
        int edit_trp();
        int edit_itm();
        int edit_critter( Creature &critter );
        int edit_veh();
        int edit_mapgen();
        void cleartmpmap( tinymap &tmpmap );
        int mapgen_preview( const real_coords &tc, uilist &gmenu );
        vehicle *mapgen_veh_query( const tripoint &omt_tgt );
        bool mapgen_veh_destroy( const tripoint &omt_tgt, vehicle *car_target );
        int mapgen_retarget();
        int select_shape( shapetype shape, int mode = -1 );

        void update_fmenu_entry( uilist &fmenu, field &field, field_type_id idx );
        void setup_fmenu( uilist &fmenu );
        catacurses::window w_info;
        catacurses::window w_help;
        int width;
        int height;
        int offsetX;
        int infoHeight;

        int ter_frn_mode;
        ter_id sel_ter;
        ter_id target_ter;
        furn_id sel_frn;
        furn_id target_frn;

        tripoint recalc_target( shapetype shape );
        bool move_target( const std::string &action, int moveorigin = -1 );
        field *cur_field;

        trap_id cur_trap;

        int sel_field;
        int sel_field_intensity;

        trap_id sel_trap;

        furn_id fsel;
        furn_id fset;
        trap_id trsel;
        trap_id trset;

        tripoint target;
        tripoint origin;
        bool moveall;
        bool refresh_mplans;
        shapetype editshape;

        std::string padding;

        std::map<field_type_id, std::string> fids;

        std::vector<tripoint> target_list;
        std::map<std::string, editmap_hilight> hilights;
        bool blink;
        bool altblink;
        point tmax;
        bool uberdraw;

        editmap();
        ~editmap();
};

#endif
