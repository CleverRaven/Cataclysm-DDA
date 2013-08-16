#ifndef _EDITMAP_H_
#define _EDITMAP_H_
#include "game.h"
#include "line.h"
#include "options.h"
#include <vector>
#include <map>
#include <list>
#include <stdarg.h>
enum shapetype {
    editmap_rect, editmap_rect_filled, editmap_line, editmap_circle,
};
class editmap
{
    public:
        void uphelp(std::string txt1 = "", std::string txt2 = "" );
        point pos2screen( const int x, const int y );
        point screen2pos( const int i, const int j );
        bool eget_direction ( int &x, int &y, InputEvent &input, int ch );
        point edit(point coords);
        void update_view(bool update_info = false);
        int edit_ter(point coords);

        int edit_fld(point coords);
        int edit_trp(point coords);
        int edit_itm(point coords);
        int edit_mon(point coords);
        int edit_npc(point coords);
        int select_shape(shapetype shape);
        void update_fmenu_entry(uimenu *fmenu, field *field, int idx);
        void setup_fmenu(uimenu *fmenu, field *field);
        bool change_fld(std::vector<point> coords, field_id fid, int density);
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


        field *cur_field;

        trap_id cur_trap;

        int sel_field;
        int sel_fdensity;

        int sel_trap;


        int fsel;
        int fset;
        int trsel;
        int trset;

        point target;
        point origin;
        bool moveall;
        shapetype editshape;

        game *g;

        std::string padding;

        std::string fids[num_fields];

        std::vector<point> target_list;
        int lastop;
        bool blink;
        bool altblink;
        int tmaxx;
        int tmaxy;
        editmap(game *gptr) {
            g = gptr;
#ifdef backport
            width = OPTIONS[OPT_SIDEBAR_STYLE] ? 45 : 55;
#else
            width = OPTIONS["SIDEBAR_STYLE"] ? 45 : 55;
#endif
            sel_ter = -1;
            target_ter = -1;
            sel_frn = -1;
            target_frn = -1;
            ter_frn_mode = 0;
            sel_field = -1;
            sel_fdensity = -1;
            sel_trap = -1;

            fsel = -1;
            fset = -1;
            trsel = -1;
            trset = -1;
            w_info = 0;
            lastop = 0;
            padding = std::string(width - 2, ' ');
            blink = false;
            altblink = false;
            moveall = false;
            editshape = editmap_rect_filled;

            tmaxx = getmaxx(g->w_terrain);
            tmaxy = getmaxy(g->w_terrain);
            fids[0] = "-clear-";
            fids[fd_fire_vent] = "fire_vent";
            fids[fd_push_items] = "push_items";
            fids[fd_shock_vent] = "shock_vent";
            fids[fd_acid_vent] = "acid_vent";
            target_list.clear();

        };
        ~editmap() {
            delwin(w_info);
            delwin(w_help);
        }
};

#endif
