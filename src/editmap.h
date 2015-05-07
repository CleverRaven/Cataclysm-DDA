#ifndef EDITMAP_H
#define EDITMAP_H

#include "game.h"
#include "map.h"
#include "line.h"
#include "omdata.h"
#include "ui.h"
#include "trap.h"
#include <vector>
#include <map>
#include <list>
#include <stdarg.h>

struct real_coords;

enum shapetype {
    editmap_rect, editmap_rect_filled, editmap_line, editmap_circle,
};

class editmap;
struct editmap_hilight {
    std::vector<bool> blink_interval;
    int cur_blink;
    nc_color color;
    std::map<tripoint, char> points;
    nc_color( *getbg )( nc_color );
    void setup()
    {
        getbg = ( color == c_red ? &red_background :
                  ( color == c_magenta ? &magenta_background :
                    ( color == c_cyan ? &cyan_background :
                      ( color == c_yellow ? &yellow_background : &green_background )
                    )
                  )
                );
    };
    void draw( editmap *em, bool update = false );
};

class editmap
{
    public:
        void uphelp( std::string txt1 = "", std::string txt2 = "", std::string title = "" );
        tripoint pos2screen( const tripoint &p );
        tripoint screen2pos( const tripoint &p );
        bool eget_direction( int &x, int &y, const std::string &action ) const;
        tripoint edit();
        void uber_draw_ter( WINDOW *w, map *m );
        void update_view( bool update_info = false );
        int edit_ter();

        int edit_fld();
        int edit_trp();
        int edit_itm();
        int edit_mon();
        int edit_npc();
        int edit_veh();
        int edit_mapgen();
        void cleartmpmap( tinymap &tmpmap );
        int mapgen_preview( real_coords &tc, uimenu &gmenu );
        int mapgen_retarget();
        int select_shape( shapetype shape, int mode = -1 );

        void update_fmenu_entry( uimenu *fmenu, field *field, int idx );
        void setup_fmenu( uimenu *fmenu );
        bool change_fld( std::vector<tripoint> coords, field_id fid, int density );
        WINDOW *w_info;
        WINDOW *w_help;
        int width;
        int height;
        int infoHeight;

        int ter_frn_mode;
        int sel_ter;
        int target_ter;
        int sel_frn;
        int target_frn;

        tripoint recalc_target( shapetype shape );
        bool move_target( const std::string &action, int moveorigin = -1 );
        field *cur_field;

        trap_id cur_trap;

        int sel_field;
        int sel_fdensity;

        int sel_trap;


        int fsel;
        int fset;
        int trsel;
        int trset;

        tripoint target;
        tripoint origin;
        bool moveall;
        bool refresh_mplans;
        shapetype editshape;

        std::string padding;

        std::string fids[num_fields];

        std::vector<tripoint> target_list;
        std::map<std::string, editmap_hilight> hilights;
        bool blink;
        bool altblink;
        int tmaxx;
        int tmaxy;
        int zlevel;
        bool uberdraw;
        std::map<oter_id, int> oter_special;
        editmap()
        {
            width = TERMX - TERRAIN_WINDOW_TERM_WIDTH;
            height = TERMY;
            infoHeight = 0;
            sel_ter = -1;
            target_ter = -1;
            sel_frn = -1;
            target_frn = -1;
            ter_frn_mode = 0;
            cur_field = 0;
            cur_trap = tr_null;
            sel_field = -1;
            sel_fdensity = -1;
            sel_trap = -1;

            fsel = -1;
            fset = -1;
            trsel = -1;
            trset = -1;
            w_info = 0;
            w_help = 0;
            padding = std::string( width - 2, ' ' );
            blink = false;
            altblink = false;
            moveall = false;
            editshape = editmap_rect;
            refresh_mplans = true;

            tmaxx = getmaxx( g->w_terrain );
            tmaxy = getmaxy( g->w_terrain );
            fids[0] = "-clear-";
            fids[fd_fire_vent] = "fire_vent";
            fids[fd_push_items] = "push_items";
            fids[fd_shock_vent] = "shock_vent";
            fids[fd_acid_vent] = "acid_vent";
            target_list.clear();
            hilights.clear();
            hilights["mplan"].blink_interval.push_back( true );
            hilights["mplan"].blink_interval.push_back( false );
            hilights["mplan"].cur_blink = 0;
            hilights["mplan"].color = c_red;
            hilights["mplan"].setup();

            hilights["mapgentgt"].blink_interval.push_back( true );
            hilights["mapgentgt"].blink_interval.push_back( false );
            hilights["mapgentgt"].blink_interval.push_back( false );
            hilights["mapgentgt"].cur_blink = 0;
            hilights["mapgentgt"].color = c_cyan;
            hilights["mapgentgt"].setup();

            oter_special.clear();
            zlevel = g->get_levz();
            uberdraw = false;
        };
        ~editmap()
        {
            delwin( w_info );
            delwin( w_help );
        }
};

#endif
