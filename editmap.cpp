#include "game.h"
#include "input.h"
#include "keypress.h"
#include "output.h"
#include "line.h"
#include "computer.h"
#include "veh_interact.h"
#include "options.h"
#include "auto_pickup.h"
#include "mapbuffer.h"
#include "debug.h"
#include "helper.h"
#include "editmap.h"
#include "map.h"
#include "output.h"
#include "uistate.h"
#include "item_factory.h"
#include "artifact.h"
#include "trap.h"
#include "mapdata.h"
#include "overmapbuffer.h"

#include <sstream>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <math.h>
#include <vector>
#include "debug.h"
#include "artifactdata.h"

#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "
#define maplim 132
#define inbounds(x, y) (x >= 0 && x < maplim && y >= 0 && y < maplim)
#define pinbounds(p) ( p.x >= 0 && p.x < maplim && p.y >= 0 && p.y < maplim)
/*
// pending merge of absolute <=> local coordinate conversion functions
#define has_real_coords 1
*/

/*
// pending merge of item.light PR
#define item_luminance 1
*/

/*
 * map position to screen position
 */
point editmap::pos2screen( const int x, const int y )
{
    return point ( tmaxx / 2 + x - target.x, tmaxy / 2 + y - target.y );
}

/*
 * screen position to map position
 */
point editmap::screen2pos( const int i, const int j )
{
    return point (i + target.x - VIEWX, j + target.y - VIEWY);
}

/*
 * standardized escape/back up keys: esc, q, space
 */
bool menu_escape ( int ch )
{
    return ( ch == KEY_ESCAPE || ch == ' ' || ch == 'q' );
}
/*
 * get_direction with extended moving via HJKL keys
 */
bool editmap::eget_direction(int &x, int &y, InputEvent &input, int ch)
{
    x = 0;
    y = 0;
    if ( ch == 'G' || ch == '0' ) {
        x = ( g->u.posx - ( target.x ) );
        y = ( g->u.posy - ( target.y ) );
        return true;
    } else if ( ch == 'H' ) {
        x = 0 - (tmaxx / 2);
    } else if ( ch == 'J' ) {
        y = (tmaxy / 2);
    } else if ( ch == 'K' ) {
        y = 0 - (tmaxy / 2);
    } else if ( ch == 'L' ) {
        x = (tmaxx / 2);
    } else {
        get_direction(x, y, input);
        return ( x != -2 && y != -2 );
    }
    return true;
}

/*
 * update the help text, which hijacks w_info's bottom border
 */
void editmap::uphelp (std::string txt1, std::string txt2, std::string title)
{

    if ( txt1 != "" ) {
        mvwprintw(w_help, 0, 0, "%s", padding.c_str() );
        mvwprintw(w_help, 1, 0, "%s", padding.c_str() );
        mvwprintw(w_help, ( txt2 != "" ? 0 : 1 ), 0, _(txt1.c_str()));
        if ( txt2 != "" ) {
            mvwprintw(w_help, 1, 0, _(txt2.c_str()));
        }
    }
    if ( title != "" ) {
        int hwidth = getmaxx(w_help);
        for ( int i = 0; i < hwidth; i++ ) { // catacurses/sdl/etc have no hline()
            mvwaddch(w_help, 2, i, LINE_OXOX);
        }
        int starttxt = int( (hwidth - title.size() - 4) / 2 );
        mvwprintw(w_help, 2, starttxt, "< ");
        wprintz(w_help, c_cyan, "%s", title.c_str() );
        wprintw(w_help, " >");
    }
    //mvwprintw(w_help, 0, 0, "%d,%d / %d,%d", target.x, target.y, origin.x, origin.y );
    wrefresh(w_help);
}


/*
 * main()
 */

point editmap::edit(point coords)
{
    target.x = g->u.posx + g->u.view_offset_x;
    target.y = g->u.posy + g->u.view_offset_y;
    int ch;
    InputEvent input;

    infoHeight = 14;

    w_info = newwin(infoHeight, width, TERMY - infoHeight, TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X);
    w_help = newwin(3, width - 2, TERMY - 3, TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X + 1);
    for ( int i = 0; i < getmaxx(w_help); i++ ) {
        mvwaddch(w_help, 2, i, LINE_OXOX);
    }

    do {
        if ( target_list.size() < 1 ) {
            target_list.push_back(target); // 'editmap.target_list' always has point 'editmap.target' at least
        }
        if ( target_list.size() == 1 ) {
            origin = target;               // 'editmap.origin' only makes sense if we have a list of target points.
        }
        update_view(true);
        uphelp("[t]rap, [f]ield, [HJKL] move++, [v] showall", "[g] terrain/furniture, [i]tems, [q]uit", "Looking around");
        ch = (int)getch();

        if(ch) {
            input = get_input(ch); // get_input: Not very useful for arbitrary keys, so check getch value first.
        }
        if(ch == 'g') {
            edit_ter( target );
            lastop = 'g';
        } else if ( ch == 'f' ) {
            edit_fld( target );
            lastop = 'f';
        } else if ( ch == 'i' ) {
            edit_itm( target );
            lastop = 'i';
        } else if ( ch == 't' ) {
            edit_trp( target );
            lastop = 't';
        } else if ( ch == 'v' ) {
            uberdraw = !uberdraw;
        } else if ( ch == 'o' ) {
            apply_mapgen( target );
            lastop = 'o';
            target_list.clear();
            origin = target;
            target_list.push_back( target);
        } else {
            if ( move_target(input, ch, 1) == true ) {
                recalc_target(editshape);           // target_list must follow movement
                if (target_list.size() > 1 ) {
                    blink = true;                       // display entire list if it's more than just target point
                }
            }
        }
    } while (input != Close && input != Cancel && ch != 'q');

    if (input == Confirm) {
        return point(target.x, target.y);
    }
    return point(-1, -1);
}


// pending radiation / misc edit
enum edit_drawmode {
    drawmode_default, drawmode_radiation,
};

/*
 * This is like game::draw_ter except it completely ignores line of sight, lighting, boomered, etc.
 * This is a map editor after all.
 */

void editmap::uber_draw_ter( WINDOW *w, map *m )
{
    point center = target;
    point start = point(center.x - getmaxx(w) / 2, center.y - getmaxy(w) / 2);
    point end = point(center.x + getmaxx(w) / 2, center.y + getmaxy(w) / 2);
    /*
        // pending filter options
        bool draw_furn=true;
        bool draw_ter=true;
        bool draw_trp=true;
        bool draw_fld=true;
        bool draw_veh=true;
    */
    bool draw_itm = true;
    bool game_map = ( ( m == &g->m || w == g->w_terrain ) ? true : false );
#ifdef has_real_coords
    const int msize = m->mapsize() * 12; // autocalc is required for mapgen stamp, which requires real_coords sanity
#else
    const int msize = SEEX * MAPSIZE;
#endif
    for (int x = start.x, sx = 0; x <= end.x; x++, sx++) {
        for (int y = start.y, sy = 0; y <= end.y; y++, sy++) {
            nc_color col = c_dkgray;
            long sym = ( game_map ? '%' : ' ' );
            if ( x >= 0 && x < msize && y >= 0 && y < msize ) {
                if ( game_map ) {
                    int mon_idx = g->mon_at(x, y);
                    int npc_idx = g->npc_at(x, y);
                    if ( mon_idx >= 0 ) {
                        g->z[mon_idx].draw(w, x, y, true);
                    } else if ( npc_idx >= 0 ) {
                        g->active_npc[npc_idx]->draw(w, x, y, true);
                    } else {
                        m->drawsq(w, g->u, x, y, false, draw_itm, center.x, center.y, false, true);
                    }
                } else {
                    m->drawsq(w, g->u, x, y, false, draw_itm, center.x, center.y, false, true);
                }
            } else {
                mvwputch(w, sy, sx, col, sym);
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// redraw map and info (or not)
void editmap::update_view(bool update_info)
{
    // Debug helper 2, child of debug helper
    // Gather useful data
    int veh_part = 0;
    vehicle *veh = g->m.veh_at(target.x, target.y, veh_part);
    int veh_in = -1;
    if(veh) {
        veh_in = veh->is_inside(veh_part);
    }

    int off = 1;

    target_ter = g->m.ter(target.x, target.y);
    ter_t terrain_type = terlist[target_ter];
    target_frn = g->m.furn(target.x, target.y);
    furn_t furniture_type = furnlist[target_frn];

    cur_field = &g->m.field_at(target.x, target.y);
    cur_trap = g->m.tr_at(target.x, target.y);
    int mon_index = g->mon_at(target.x, target.y);
    int npc_index = g->npc_at(target.x, target.y);

    // update map always
    werase(g->w_terrain);

    if ( uberdraw ) {
        uber_draw_ter( g->w_terrain, &g->m ); // Bypassing the usual draw methods; not versatile enough
    } else {
        g->draw_ter(target.x, target.y);      // But it's optional
    }

    // update target point
    if (mon_index != -1) {
        g->z[mon_index].draw(g->w_terrain, target.x, target.y, true);
    } else if (npc_index != -1) {
        g->active_npc[npc_index]->draw(g->w_terrain, target.x, target.y, true);
    } else {
        g->m.drawsq(g->w_terrain, g->u, target.x, target.y, true, true, target.x, target.y);
    }

    // hilight target_list points if blink=true (and if it's more than a point )
    if ( blink && target_list.size() > 1 ) {
        for ( int i = 0; i < target_list.size(); i++ ) {
            int x = target_list[i].x;
            int y = target_list[i].y;
            int vpart = 0;
            // but only if there's no vehicles/mobs/npcs on a point
            if ( ! g->m.veh_at(x, y, vpart) && ( g->mon_at(x, y) == -1 ) && ( g->npc_at(x, y) == -1 ) ) {
                char t_sym = terlist[g->m.ter(x, y)].sym;
                nc_color t_col = terlist[g->m.ter(x, y)].color;


                if ( g->m.furn(x, y) > 0 ) {
                    furn_t furniture_type = furnlist[g->m.furn(x, y)];
                    t_sym = furniture_type.sym;
                    t_col = furniture_type.color;
                }
                field *t_field = &g->m.field_at(x, y);
                if ( t_field->fieldCount() > 0 ) {
                    field_id t_ftype = t_field->fieldSymbol();
                    field_entry *t_fld = t_field->findField( t_ftype );
                    if ( t_fld != NULL ) {
                        t_col =  fieldlist[t_ftype].color[t_fld->getFieldDensity()-1];
                        t_sym = fieldlist[t_ftype].sym;
                    }
                }
                t_col = ( altblink == true ? green_background ( t_col ) : cyan_background ( t_col ) );
                point scrpos = pos2screen( x, y );
                mvwputch(g->w_terrain, scrpos.y, scrpos.x, t_col, t_sym);
            }
        }
    }

    // draw arrows if altblink is set (ie, [m]oving a large selection
    if ( blink && altblink ) {
        int mpx = (tmaxx / 2) + 1;
        int mpy = (tmaxy / 2) + 1;
        mvwputch(g->w_terrain, mpy, 1, c_yellow, '<');
        mvwputch(g->w_terrain, mpy, tmaxx - 1, c_yellow, '>');
        mvwputch(g->w_terrain, 1, mpx, c_yellow, '^');
        mvwputch(g->w_terrain, tmaxy - 1, mpx, c_yellow, 'v');
    }

    wrefresh(g->w_terrain);

    if ( update_info ) { // only if requested; this messes up windows layered ontop
        wborder(w_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

#ifdef has_real_coords   // display real values if we can
        real_coords rc(g->levx, g->levy, target.x, target.y);
        rc.fromabs(g->m.getabs(target.x, target.y));

        mvwprintz(w_info, 0, 2 , c_ltgray, "< %d,%d %d,%d / %d,%d %d,%d %d,%d >--",
                  g->m.get_abs_sub().x, g->m.get_abs_sub().y, target.x, target.y,
                  rc.abs_pos.x, rc.abs_pos.y, rc.abs_sub.x, rc.abs_sub.y, rc.abs_om.x, rc.abs_om.y
                 );
#else
        mvwprintz(w_info, 0, 2 , c_ltgray, "< %d,%d >--", target.x, target.y);
#endif
        for (int i = 1; i < infoHeight - 2; i++) { // clear window
            mvwprintz(w_info, i, 1, c_white, padding.c_str());
        }

        mvwputch(w_info, off, 2, terrain_type.color, terrain_type.sym);
        mvwprintw(w_info, off, 4, _("%d: %s; movecost %d"), g->m.ter(target.x, target.y),
                  terrain_type.name.c_str(),
                  terrain_type.movecost
                 );
        off++; // 2
        if ( g->m.furn(target.x, target.y) > 0 ) {
            mvwputch(w_info, off, 2, furniture_type.color, furniture_type.sym);
            mvwprintw(w_info, off, 4, _("%d: %s; movecost %d movestr %d"), g->m.furn(target.x, target.y),
                      furniture_type.name.c_str(),
                      furniture_type.movecost,
                      furniture_type.move_str_req
                     );
            off++; // 3
        }
        mvwprintw(w_info, off, 2, _("dist: %d u_see: %d light: %d v_in: %d"), rl_dist(g->u.posx, g->u.posy, target.x, target.y), g->u_see(target.x, target.y), g->m.light_at(target.x, target.y), veh_in );
        off++; // 3-4

        std::string extras = "";
        if(veh_in >= 0) {
            extras += _(" [vehicle]");
        }
        if(g->m.has_flag(indoors, target.x, target.y)) {
            extras += _(" [indoors]");
        }
        if(g->m.has_flag(supports_roof, target.x, target.y)) {
            extras += _(" [roof]");
        }

        mvwprintw(w_info, off, 1, "%s %s", g->m.features(target.x, target.y).c_str(), extras.c_str());
        off++;  // 4-5

        if (cur_field->fieldCount() > 0) {
            field_entry *cur = NULL;
            for(std::map<field_id, field_entry *>::iterator field_list_it = cur_field->getFieldStart(); field_list_it != cur_field->getFieldEnd(); ++field_list_it) {
                cur = field_list_it->second;
                if(cur == NULL) {
                    continue;
                }
                mvwprintz(w_info, off, 1, fieldlist[cur->getFieldType()].color[cur->getFieldDensity()-1], _("field: %s (%d) density %d age %d"),
                          fieldlist[cur->getFieldType()].name[cur->getFieldDensity()-1].c_str(), cur->getFieldType(), cur->getFieldDensity(), cur->getFieldAge()
                         );
                off++; // 5ish
            }
        }


        if (cur_trap != tr_null) {
            mvwprintz(w_info, off, 1, g->traps[cur_trap]->color, _("trap: %s (%d)"),
                      g->traps[cur_trap]->name.c_str(), cur_trap
                     );
            off++; // 6
        }

        if (mon_index != -1) {
            g->z[mon_index].print_info(g, w_info);
            off += 6;
        } else if (npc_index != -1) {
            g->active_npc[npc_index]->print_info(w_info);
            off += 6;
        } else if (veh) {
            mvwprintw(w_info, off, 1, _("There is a %s there. Parts:"), veh->name.c_str());
            off++;
            veh->print_part_desc(w_info, off, width, veh_part);
            off += 6;
        }

        if (!g->m.has_flag(container, target.x, target.y) && g->m.i_at(target.x, target.y).size() > 0) {
            mvwprintw(w_info, off, 1, _("There is a %s there."),
                      g->m.i_at(target.x, target.y)[0].tname(g).c_str());
            off++;
            if (g->m.i_at(target.x, target.y).size() > 1) {
                mvwprintw(w_info, off, 1, _("There are %d other items there as well."), g->m.i_at(target.x, target.y).size() - 1);
                off++;
            }
        }


        if (g->m.graffiti_at(target.x, target.y).contents) {
            mvwprintw(w_info, off, 1, _("Graffiti: %s"), g->m.graffiti_at(target.x, target.y).contents->c_str());
        }
        off++;


        wrefresh(w_info);

        uphelp();
    }

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// edit terrain type / furniture
int editmap::edit_ter(point coords)
{
    int ret = 0;
    int pwh = TERMY - 4;

    WINDOW *w_pickter = newwin(pwh, width, VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X);
    wborder(w_pickter, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wrefresh(w_pickter);

    int pickh = pwh - 2;
    int pickw = width - 4;
    int cur_t = 0;
    int cur_f = 0;

    if( sel_ter < 0 ) {
        sel_ter = target_ter;
    }

    if( sel_frn < 0 ) {
        sel_frn = target_frn;
    }

    int lastsel_ter = sel_ter;
    int lastsel_frn = sel_frn;

    int xmax = pickw;
    int tymax = int(num_terrain_types / xmax);
    if ( tymax % xmax != 0 ) {
        tymax++;
    }
    int fymax = int(num_furniture_types / xmax);
    if ( fymax == 0 || fymax % xmax != 0 ) {
        fymax++;
    }

    int subch = 0;
    point sel_terp = point(-1, -1);     // screen coords of current selection
    point lastsel_terp = point(-1, -1); // and last selection
    point target_terp = point(-1, -1);  // and current tile
    point sel_frnp = point(-1, -1);     // for furniture ""
    point lastsel_frnp = point(-1, -1);
    point target_frnp = point(-1, -1);


    int mode = ter_frn_mode;
    do {
        if ( mode != ter_frn_mode ) {
            mode = ter_frn_mode;
            wrefresh(w_pickter);
        }

        // cursor is green for terrain or furniture, depending on selection
        nc_color c_tercurs = ( ter_frn_mode == 0 ? c_ltgreen : c_dkgray );
        nc_color c_frncurs = ( ter_frn_mode == 1 ? c_ltgreen : c_dkgray );

        cur_t = 0;
        int tstart = 2;
        // draw icon grid
        for (int y = tstart; y < pickh && cur_t < num_terrain_types; y += 2) {
            for (int x = 3; x < pickw && cur_t < num_terrain_types; x++, cur_t++) {
                ter_t ttype = terlist[cur_t];
                mvwputch(w_pickter, y, x, ( ter_frn_mode == 0 ? ttype.color : c_dkgray ) , ttype.sym);
                if(cur_t == sel_ter) {
                    sel_terp = point(x, y);
                } else if(cur_t == lastsel_ter) {
                    lastsel_terp = point(x, y);
                } else if (cur_t == target_ter) {
                    target_terp = point(x, y);
                }
            }
        }
        // clear last cursor area
        mvwputch(w_pickter, lastsel_terp.y + 1, lastsel_terp.x - 1, c_tercurs, ' ');
        mvwputch(w_pickter, lastsel_terp.y - 1, lastsel_terp.x + 1, c_tercurs, ' ');
        mvwputch(w_pickter, lastsel_terp.y + 1, lastsel_terp.x + 1, c_tercurs, ' ');
        mvwputch(w_pickter, lastsel_terp.y - 1, lastsel_terp.x - 1, c_tercurs, ' ');
        // indicate current tile
        mvwputch(w_pickter, target_terp.y + 1, target_terp.x, c_ltgray, '^');
        mvwputch(w_pickter, target_terp.y - 1, target_terp.x, c_ltgray, 'v');
        // draw cursor around selected terrain icon
        mvwputch(w_pickter, sel_terp.y + 1, sel_terp.x - 1, c_tercurs, LINE_XXOO);
        mvwputch(w_pickter, sel_terp.y - 1, sel_terp.x + 1, c_tercurs, LINE_OOXX);
        mvwputch(w_pickter, sel_terp.y + 1, sel_terp.x + 1, c_tercurs, LINE_XOOX);
        mvwputch(w_pickter, sel_terp.y - 1, sel_terp.x - 1, c_tercurs, LINE_OXXO);

        wborder(w_pickter, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
        // calc offset, print terrain selection info
        int tlen = tymax * 2;
        int off = tstart + tlen;
        mvwprintw(w_pickter, off, 1, "%s", padding.c_str());
        if( ter_frn_mode == 0 ) { // unless furniture is selected
            ter_t pttype = terlist[sel_ter];

            mvwprintw(w_pickter, 0, 2, "< %d: %s >-----------", sel_ter, pttype.name.c_str());
            mvwprintz(w_pickter, off, 2, c_white, _("movecost %d"), pttype.movecost);
            std::string extras = "";
            if(pttype.flags & mfb(indoors)) {
                extras += _("[indoors] ");
            }
            if(pttype.flags & mfb(supports_roof)) {
                extras += _("[roof] ");
            }
            wprintw(w_pickter, " %s", extras.c_str());
        }

        off += 2;
        cur_f = 0;
        int fstart = off; // calc vertical offset, draw furniture icons
        for (int y = fstart; y < pickh && cur_f < num_furniture_types; y += 2) {
            for (int x = 3; x < pickw && cur_f < num_furniture_types; x++, cur_f++) {

                furn_t ftype = furnlist[cur_f];
                mvwputch(w_pickter, y, x, ( ter_frn_mode == 1 ? ftype.color : c_dkgray ), ftype.sym);

                if(cur_f == sel_frn) {
                    sel_frnp = point(x, y);
                } else if(cur_f == lastsel_frn) {
                    lastsel_frnp = point(x, y);
                } else if (cur_f == target_frn) {
                    target_frnp = point(x, y);
                }
            }
        }

        mvwputch(w_pickter, lastsel_frnp.y + 1, lastsel_frnp.x - 1, c_frncurs, ' ');
        mvwputch(w_pickter, lastsel_frnp.y - 1, lastsel_frnp.x + 1, c_frncurs, ' ');
        mvwputch(w_pickter, lastsel_frnp.y + 1, lastsel_frnp.x + 1, c_frncurs, ' ');
        mvwputch(w_pickter, lastsel_frnp.y - 1, lastsel_frnp.x - 1, c_frncurs, ' ');

        mvwputch(w_pickter, target_frnp.y + 1, target_frnp.x, c_ltgray, '^');
        mvwputch(w_pickter, target_frnp.y - 1, target_frnp.x, c_ltgray, 'v');

        mvwputch(w_pickter, sel_frnp.y + 1, sel_frnp.x - 1, c_frncurs, LINE_XXOO);
        mvwputch(w_pickter, sel_frnp.y - 1, sel_frnp.x + 1, c_frncurs, LINE_OOXX);
        mvwputch(w_pickter, sel_frnp.y + 1, sel_frnp.x + 1, c_frncurs, LINE_XOOX);
        mvwputch(w_pickter, sel_frnp.y - 1, sel_frnp.x - 1, c_frncurs, LINE_OXXO);

        int flen = fymax * 2;
        off += flen;
        mvwprintw(w_pickter, off, 1, "%s", padding.c_str());
        if( ter_frn_mode == 1 ) {

            furn_t pftype = furnlist[sel_frn];

            mvwprintw(w_pickter, 0, 2, "< %d: %s >-----------", sel_frn, pftype.name.c_str());
            mvwprintz(w_pickter, off, 2, c_white, _("movecost %d"), pftype.movecost);
            std::string fextras = "";
            if(pftype.flags & mfb(indoors)) {
                fextras += _("[indoors] ");
            }
            if(pftype.flags & mfb(supports_roof)) {
                fextras += _("[roof] ");
            }
            wprintw(w_pickter, " %s", fextras.c_str());
        }

        // draw green |'s around terrain or furniture tilesets depending on selection
        for (int y = tstart - 1; y < tstart + tlen + 1; y++ ) {
            mvwputch(w_pickter, y, 1, c_ltgreen, ( ter_frn_mode == 0 ? '|' : ' ' ) );
            mvwputch(w_pickter, y, width - 2, c_ltgreen, ( ter_frn_mode == 0 ? '|' : ' ' ) );
        }
        for (int y = fstart - 1; y < fstart + flen + 1; y++ ) {
            mvwputch(w_pickter, y, 1, c_ltgreen, ( ter_frn_mode == 1 ? '|' : ' ' ) );
            mvwputch(w_pickter, y, width - 2, c_ltgreen, ( ter_frn_mode == 1 ? '|' : ' ' ) );
        }

        uphelp("[s/tab] shape select, [m]ove, [<>^v] select",
               "[enter] change, [g] change/quit, [q]uit",
               "Terrain / Furniture");

        wrefresh(w_pickter);

        subch = (int)getch();
        lastsel_ter = sel_ter;
        lastsel_frn = sel_frn;
        if ( ter_frn_mode == 0 ) {
            if( subch == KEY_LEFT ) {
                sel_ter = (sel_ter - 1 >= 0 ? sel_ter - 1 : num_terrain_types - 1);
            } else if( subch == KEY_RIGHT ) {
                sel_ter = (sel_ter + 1 < num_terrain_types ? sel_ter + 1 : 0 );
            } else if( subch == KEY_UP ) {
                if (sel_ter - xmax + 3 > 0 ) {
                    sel_ter = sel_ter - xmax + 3;
                } else {
                    ter_frn_mode = ( ter_frn_mode == 0 ? 1 : 0 );
                }
            } else if( subch == KEY_DOWN ) {
                if (sel_ter + xmax - 3 < num_terrain_types ) {
                    sel_ter = sel_ter + xmax - 3;
                } else {
                    ter_frn_mode = ( ter_frn_mode == 0 ? 1 : 0 );
                }
            } else if( subch == KEY_ENTER || subch == '\n' || subch == 'g' ) {
                ter_t tset = terlist[sel_ter];
                for(int t = 0; t < target_list.size(); t++ ) {
                    g->m.ter_set(target_list[t].x, target_list[t].y, (ter_id)sel_ter);
                }
                if ( subch == 'g' ) {
                    subch = KEY_ESCAPE;
                }
                update_view(false);
            } else if ( subch == 's'  || subch == '\t' || subch == 'm'  ) {
                int sel_tmp = sel_ter;
                select_shape(editshape, ( subch == 'm' ? 1 : 0 ) );
                sel_ter = sel_tmp;
            }
        } else { // todo: cleanup
            if( subch == KEY_LEFT ) {
                sel_frn = (sel_frn - 1 >= 0 ? sel_frn - 1 : num_furniture_types - 1);
            } else if( subch == KEY_RIGHT ) {
                sel_frn = (sel_frn + 1 < num_furniture_types ? sel_frn + 1 : 0 );
            } else if( subch == KEY_UP ) {
                if ( sel_frn - xmax + 3 > 0 ) {
                    sel_frn = sel_frn - xmax + 3;
                } else {
                    ter_frn_mode = ( ter_frn_mode == 0 ? 1 : 0 );
                }
            } else if( subch == KEY_DOWN ) {
                if ( sel_frn + xmax - 3 < num_furniture_types ) {
                    sel_frn = sel_frn + xmax - 3;
                } else {
                    ter_frn_mode = ( ter_frn_mode == 0 ? 1 : 0 );
                }
            } else if( subch == KEY_ENTER || subch == '\n' || subch == 'g' ) {
                furn_t tset = furnlist[sel_frn];
                for(int t = 0; t < target_list.size(); t++ ) {
                    g->m.furn_set(target_list[t].x, target_list[t].y, (furn_id)sel_frn);
                }
                if ( subch == 'g' ) {
                    subch = KEY_ESCAPE;
                }
                update_view(false);
            } else if ( subch == 's' || subch == '\t' || subch == 'm' ) {
                int sel_frn_tmp = sel_frn;
                int sel_ter_tmp = sel_ter;
                select_shape(editshape, ( subch == 'm' ? 1 : 0 ) );
                sel_frn = sel_frn_tmp;
                sel_ter = sel_ter_tmp;
            }
        }
    } while ( ! menu_escape ( subch ) );

    werase(w_pickter);
    wrefresh(w_pickter);

    delwin(w_pickter);
    return ret;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// field edit

void editmap::update_fmenu_entry(uimenu *fmenu, field *field, int idx)
{
    int fdens = 1;
    field_t ftype = fieldlist[idx];
    field_entry *fld = field->findField((field_id)idx);
    if ( fld != NULL ) {
        fdens = fld->getFieldDensity();
    }
    fmenu->entries[idx].txt = ( ftype.name[fdens-1].size() == 0 ? fids[idx] : ftype.name[fdens-1] );
    if ( fld != NULL ) {
        fmenu->entries[idx].txt += " " + std::string(fdens, '*');
    }
    fmenu->entries[idx].text_color = ( fld != NULL ? c_cyan : fmenu->text_color );
    fmenu->entries[idx].extratxt.color = ftype.color[fdens-1];
}

void editmap::setup_fmenu(uimenu *fmenu, field *field)
{
    std::string fname;
    fmenu->entries.clear();
    for ( int i = 0; i < num_fields; i++ ) {
        field_t ftype = fieldlist[i];
        int fdens = 1;
        fname = ( ftype.name[fdens-1].size() == 0 ? fids[i] : ftype.name[fdens-1] );
        fmenu->addentry(i, true, -2, "%s", fname.c_str());
        fmenu->entries[i].extratxt.left = 1;
        fmenu->entries[i].extratxt.txt = string_format("%c", ftype.sym);
        update_fmenu_entry( fmenu, cur_field, i );
    }
    if ( sel_field >= 0 ) {
        fmenu->selected = sel_field;
    }
}

int editmap::edit_fld(point coords)
{
    int ret = 0;
    uimenu fmenu;
    fmenu.w_width = width;
    fmenu.w_height = TERMY - infoHeight;
    fmenu.w_y = 0;
    fmenu.w_x = TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X;
    fmenu.return_invalid = true;
    setup_fmenu(&fmenu, cur_field);

    do {
        uphelp("[s/tab] shape select, [m]ove, [<,>] density",
               "[enter] edit, [q]uit", "Field effects");

        fmenu.query(false);
        if ( fmenu.selected > 0 && fmenu.selected < num_fields &&
             ( fmenu.keypress == '\n' || fmenu.keypress == KEY_LEFT || fmenu.keypress == KEY_RIGHT )
           ) {
            int fdens = 0;
            int idx = fmenu.selected;
            field_entry *fld = cur_field->findField((field_id)idx);
            if ( fld != NULL ) {
                fdens = fld->getFieldDensity();
            }
            int fsel_dens = fdens;
            if ( fmenu.keypress == '\n' ) {
                uimenu femenu;
                femenu.w_width = width;
                femenu.w_height = infoHeight;
                femenu.w_y = fmenu.w_height;
                femenu.w_x = TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X;

                femenu.return_invalid = true;
                field_t ftype = fieldlist[idx];
                int fidens = ( fdens == 0 ? 0 : fdens - 1 );
                femenu.text = ( ftype.name[fidens].size() == 0 ? fids[idx] : ftype.name[fidens] );
                femenu.addentry("-clear-");

                femenu.addentry("1: %s", ( ftype.name[0].size() == 0 ? fids[idx].c_str() : ftype.name[0].c_str() ));
                femenu.addentry("2: %s", ( ftype.name[1].size() == 0 ? fids[idx].c_str() : ftype.name[1].c_str() ));
                femenu.addentry("3: %s", ( ftype.name[2].size() == 0 ? fids[idx].c_str() : ftype.name[2].c_str() ));
                femenu.entries[fdens].text_color = c_cyan;
                femenu.selected = ( sel_fdensity > 0 ? sel_fdensity : fdens );

                femenu.query();
                if ( femenu.ret >= 0 ) {
                    fsel_dens = femenu.ret;
                }
            } else if ( fmenu.keypress == KEY_RIGHT && fdens < 3 ) {
                fsel_dens++;
            } else if ( fmenu.keypress == KEY_LEFT && fdens > 0 ) {
                fsel_dens--;
            }
            if ( fdens != fsel_dens || target_list.size() > 1 ) {
                for(int t = 0; t < target_list.size(); t++ ) {
                    field *t_field = &g->m.field_at(target_list[t].x, target_list[t].y);
                    field_entry *t_fld = t_field->findField((field_id)idx);
                    int t_dens = 0;
                    if ( t_fld != NULL ) {
                        t_dens = t_fld->getFieldDensity();
                    }
                    if ( fsel_dens != 0 ) {
                        if ( t_dens != 0 ) {
                            t_fld->setFieldDensity(fsel_dens);
                        } else {
                            g->m.add_field(g, target_list[t].x, target_list[t].y, (field_id)idx, fsel_dens );
                        }
                    } else {
                        if ( t_dens != 0 ) {
                            t_field->removeField( (field_id)idx );
                        }
                    }
                }
                update_fmenu_entry( &fmenu, cur_field, idx );
                update_view(true);
                sel_field = fmenu.selected;
                sel_fdensity = fsel_dens;
            }
        } else if ( fmenu.selected == 0 && fmenu.keypress == '\n' ) {
            for(int t = 0; t < target_list.size(); t++ ) {
                field *t_field = &g->m.field_at(target_list[t].x, target_list[t].y);
                if ( t_field->fieldCount() > 0 ) {
                    for ( std::map<field_id, field_entry *>::iterator field_list_it = t_field->getFieldStart();
                          field_list_it != t_field->getFieldEnd(); ++field_list_it
                        ) {
                        field_id rmid = field_list_it->first;
                        t_field->removeField( rmid );
                        if ( target_list[t].x == target.x && target_list[t].y == target.y ) {
                            update_fmenu_entry( &fmenu, t_field, (int)rmid );
                        }
                    }
                }
            }
            update_view(true);
            sel_field = fmenu.selected;
            sel_fdensity = 0;
        } else if ( fmenu.keypress == 's' || fmenu.keypress == '\t' || fmenu.keypress == 'm' ) {
            int sel_tmp = fmenu.selected;
            int ret = select_shape(editshape, ( fmenu.keypress == 'm' ? 1 : 0 ) );
            if ( ret > 0 ) {
                setup_fmenu(&fmenu, cur_field);
            }
            fmenu.selected = sel_tmp;
        }
    } while ( ! menu_escape ( fmenu.keypress ) );
    wrefresh(w_info);
    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// edit traps
int editmap::edit_trp(point coords)
{
    int ret = 0;
    int pwh = TERMY - infoHeight - 1;

    WINDOW *w_picktrap = newwin(pwh, width, VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X);
    wborder(w_picktrap, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    int tmax = pwh - 4;
    int tshift = 0;
    int subch = 0;
    if ( trsel == -1 ) {
        trsel = cur_trap;
    }
    std::string trids[num_trap_types];
    trids[0] = _("-clear-");
    do {
        uphelp("[s/tab] shape select, [m]ove",
               "[enter] change, [t] change/quit, [q]uit", "Traps");

        if( trsel < tshift ) {
            tshift = trsel;
        } else if ( trsel > tshift + tmax ) {
            tshift = trsel - tmax;
        }
        std::string tnam;
        for ( int t = tshift; t <= tshift + tmax; t++ ) {
            mvwprintz(w_picktrap, t + 1 - tshift, 1, c_white, "%s", padding.c_str());
            if ( t < num_trap_types ) {
                tnam = ( g->traps[t]->name.size() == 0 ? trids[t] : g->traps[t]->name );
                mvwputch(w_picktrap, t + 1 - tshift, 2, g->traps[t]->color, g->traps[t]->sym);
                mvwprintz(w_picktrap, t + 1 - tshift, 4, (trsel == t ? h_white : ( cur_trap == t ? c_green : c_ltgray ) ), "%d %s", t, tnam.c_str() );
            }
        }
        wrefresh(w_picktrap);

        subch = (int)getch();
        if(subch == KEY_UP) {
            trsel--;
        } else if (subch == KEY_DOWN) {
            trsel++;
        } else if ( subch == KEY_ENTER || subch == '\n' || subch == 't' ) {
            if ( trsel < num_trap_types && trsel >= 0 ) {
                trset = trsel;
            }
            for(int t = 0; t < target_list.size(); t++ ) {
                g->m.add_trap(target_list[t].x, target_list[t].y, trap_id(trset));
            }
            if ( subch == 't' ) {
                subch = KEY_ESCAPE;
            }
            update_view(false);
        } else if ( subch == 's' || subch == '\t' || subch == 'm' ) {
            int sel_tmp = trsel;
            select_shape(editshape, ( subch == 'm' ? 1 : 0 ) );
            sel_frn = sel_tmp;
        }
        if( trsel < 0 ) {
            trsel = num_trap_types - 1;
        } else if ( trsel >= num_trap_types ) {
            trsel = 0;
        }

    } while ( ! menu_escape ( subch ) );
    werase(w_picktrap);
    wrefresh(w_picktrap);
    delwin(w_picktrap);

    wrefresh(w_info);

    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * edit items in target square. WIP
 */
enum editmap_imenu_ent {
    imenu_bday, imenu_damage, imenu_burnt,
#ifdef item_luminance
    imenu_sep, imenu_luminance, imenu_direction, imenu_width,
#endif
    imenu_exit,
};

int editmap::edit_itm(point coords)
{
    int ret = 0;
    uimenu ilmenu;
    ilmenu.w_x = TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X;
    ilmenu.w_y = 0;
    ilmenu.w_width = width;
    ilmenu.w_height = TERMY - infoHeight - 1;
    ilmenu.return_invalid = true;
    std::vector<item>& items = g->m.i_at(target.x , target.y );
    for(int i = 0; i < items.size(); i++) {
#ifdef item_luminance
        ilmenu.addentry(i, true, 0, "%s%s", items[i].tname(g).c_str(), items[i].light.luminance > 0 ? " L" : "" );
#else
        ilmenu.addentry(i, true, 0, "%s", items[i].tname(g).c_str());
#endif
    }
    // todo; ilmenu.addentry(ilmenu.entries.size(), true, 'a', "Add item");
    ilmenu.addentry(-10, true, 'q', "Cancel");
    do {
        ilmenu.query();
        if ( ilmenu.ret >= 0 && ilmenu.ret < items.size() ) {
            item *it = &items[ilmenu.ret];
            uimenu imenu;
            imenu.w_x = ilmenu.w_x;
            imenu.w_y = ilmenu.w_height;
            imenu.w_height = TERMX - ilmenu.w_height;
            imenu.w_width = ilmenu.w_width;
            imenu.addentry(imenu_bday, true, -1, "bday: %d", (int)it->bday);
            imenu.addentry(imenu_damage, true, -1, "damage: %d", (int)it->damage);
            imenu.addentry(imenu_burnt, true, -1, "burnt: %d", (int)it->burnt);
#ifdef item_luminance
            imenu.addentry(imenu_sep, false, 0, "-[ light emission ]-");
            imenu.addentry(imenu_luminance, true, -1, "lum: %f", (float)it->light.luminance);
            imenu.addentry(imenu_direction, true, -1, "dir: %d", (int)it->light.direction);
            imenu.addentry(imenu_width, true, -1, "width: %d", (int)it->light.width);
#endif
            imenu.addentry(imenu_exit, true, -1, "exit");
            do {
                imenu.query();
                if ( imenu.ret >= 0 && imenu.ret < imenu_exit ) {
                    int intval = -1;
                    switch(imenu.ret) {
                        case imenu_bday:
                            intval = (int)it->bday;
                            break;
                        case imenu_damage:
                            intval = (int)it->damage;
                            break;
                        case imenu_burnt:
                            intval = (int)it->burnt;
                            break;
#ifdef item_luminance
                        case imenu_luminance:
                            intval = (int)it->light.luminance;
                            break;
                        case imenu_direction:
                            intval = (int)it->light.direction;
                            break;
                        case imenu_width:
                            intval = (int)it->light.width;
                            break;
#endif
                    }
                    int retval = helper::to_int (
                                     string_input_popup( "set: ", 20, helper::to_string(  intval ) )
                                 );
                    if ( intval != retval ) {
                        if (imenu.ret == imenu_bday ) {
                            it->bday = retval;
                            imenu.entries[imenu_bday].txt = string_format("bday: %d", it->bday);
                        } else if (imenu.ret == imenu_damage ) {
                            it->damage = retval;
                            imenu.entries[imenu_damage].txt = string_format("damage: %d", it->damage);
                        } else if (imenu.ret == imenu_burnt ) {
                            it->burnt = retval;
                            imenu.entries[imenu_burnt].txt = string_format("burnt: %d", it->burnt);
#ifdef item_luminance
                        } else if (imenu.ret == imenu_luminance ) {
                            it->light.luminance = (unsigned short)retval;
                            imenu.entries[imenu_luminance].txt = string_format("lum: %f", (float)it->light.luminance);
                        } else if (imenu.ret == imenu_direction ) {
                            it->light.direction = (short)retval;
                            imenu.entries[imenu_direction].txt = string_format("dir: %d", (int)it->light.direction);
                        } else if (imenu.ret == imenu_width ) {
                            it->light.width = (short)retval;
                            imenu.entries[imenu_width].txt = string_format("width: %d", (int)it->light.width);
#endif
                        }
                        werase(g->w_terrain);
                        g->draw_ter(target.x, target.y);
                    }
                    wrefresh(ilmenu.window);
                    wrefresh(imenu.window);
                    wrefresh(g->w_terrain);
                }
            } while(imenu.ret != imenu_exit);
            wrefresh(w_info);
        }
    } while (ilmenu.ret >= 0 || ilmenu.ret == UIMENU_INVALID);
    return ret;

}

/*
 *  Todo
 */
int editmap::edit_mon(point coords)
{
    int ret = 0;
    return ret;
}

/*
 *  Calculate target_list based on origin and target class variables, and shapetype.
 */
point editmap::recalc_target(shapetype shape)
{
    point ret = target;
    target_list.clear();
    switch(shape) {
        case editmap_circle: {
            int radius = rl_dist(origin.x, origin.y, target.x, target.y);
            for ( int x = origin.x - radius; x <= origin.x + radius; x++ ) {
                for ( int y = origin.y - radius; y <= origin.y + radius; y++ ) {
                    if(rl_dist(x, y, origin.x, origin.y) <= radius) {
                        if ( inbounds(x, y) ) {
                            target_list.push_back(point(x, y));
                        }
                    }
                }
            }
        }
        break;
        case editmap_rect_filled:
        case editmap_rect:
            int sx;
            int sy;
            int ex;
            int ey;
            if ( target.x < origin.x ) {
                sx = target.x;
                ex = origin.x;
            } else {
                sx = origin.x;
                ex = target.x;
            }
            if ( target.y < origin.y ) {
                sy = target.y;
                ey = origin.y;
            } else {
                sy = origin.y;
                ey = target.y;
            }
            for ( int x = sx; x <= ex; x++ ) {
                for ( int y = sy; y <= ey; y++ ) {
                    if ( shape == editmap_rect_filled || x == sx || x == ex || y == sy || y == ey ) {
                        if ( inbounds(x, y) ) {
                            target_list.push_back(point(x, y));
                        }
                    }
                }
            }
            break;
        case editmap_line:
            int t = 0;
            target_list = line_to(origin.x, origin.y, target.x, target.y, t);
            break;
    }

    return ret;
}

/*
 * Shift 'var' (ie, part of a coordinate plane) by 'shift'.
 * If the result is not >= 0 and < 'max', constrain the result and adjust 'shift',
 * so it can adjust subsequent points of a set consistently.
 */
int limited_shift ( int var, int &shift, int max )
{
    if ( var + shift < 0 ) {
        shift = shift - ( var + shift );
    } else if ( var + shift >= max ) {
        shift = shift + ( max - 1 - ( var + shift ) );
    }
    return var += shift;
}

/*
 * Move point 'editmap.target' via keystroke. 'moveorigin' determines if point 'editmap.origin' is moved as well:
 * 0: no, 1: yes, -1 (or none): as per bool 'editmap.moveall'.
 * if input or ch are not valid movement keys, do nothing and return false
 */
bool editmap::move_target( InputEvent &input, int ch, int moveorigin )
{
    int mx, my;
    bool move_origin = ( moveorigin == 1 ? true : ( moveorigin == 0 ? false : moveall ) );
    if ( eget_direction(mx, my, input, ch ) == true ) {
        target.x = limited_shift ( target.x, mx, maplim );
        target.y = limited_shift ( target.y, my, maplim );
        if ( move_origin ) {
            origin.x += mx;
            origin.y += my;
        }
        return true;
    }
    return false;
}

/*
 * Todo
 */
int editmap::edit_npc(point coords)
{
    int ret = 0;
    return ret;
}

/*
 * Interactively select, resize, and move the list of target coords
 */
int editmap::select_shape(shapetype shape, int mode)
{
    point orig = target;
    point origor = origin;
    int ch = 0;
    InputEvent input;
    bool update = false;
    blink = true;
    if ( mode >= 0 ) {
        moveall = ( mode == 0 ? false : true );
    }
    altblink = moveall;
    update_view(false);
    timeout(BLINK_SPEED);
    do {
        uphelp(
            ( moveall == true ? "[s] resize, [y] swap" :
              "[m]move, [s]hape, [y] swap, [z] to start" ),
            "[enter] accept, [q] abort",
            ( moveall == true ? "Moving selection" : "Resizing selection" ) );
        ch = getch();
        timeout(BLINK_SPEED);
        if(ch != ERR) {
            blink = true;
            input = get_input(ch);
            if(ch == 's') {
                if ( ! moveall ) {
                    timeout(-1);
                    uimenu smenu;
                    smenu.text = "Selection type";
                    smenu.w_x = (TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X - 16) / 2;
                    smenu.addentry(editmap_rect, true, 'r', "Rectangle");
                    smenu.addentry(editmap_rect_filled, true, 'f', "Filled Rectangle");
                    smenu.addentry(editmap_line, true, 'l', "Line");
                    smenu.addentry(editmap_circle, true, 'c', "Filled Circle");
                    smenu.addentry(-2, true, 'p', "Point");
                    smenu.selected = (int)shape;
                    smenu.query();
                    if ( smenu.ret != -2 ) {
                        shape = (shapetype)smenu.ret;
                        update = true;
                    } else {
                        target_list.clear();
                        origin = target;
                        target_list.push_back(target);
                        moveall = true;
                    }
                    timeout(BLINK_SPEED);
                } else {
                    moveall = false;
                }
            } else if ( moveall == false && ch == 'z' ) {
                target = origin;
                update = true;
            } else if ( ch == 'y' ) {
                point tmporigin = origin;
                origin = target;
                target = tmporigin;
                update = true;
            } else if ( ch == 'm' ) {
                moveall = true;
            } else if ( ch == '\t' ) {
                if ( moveall ) {
                    moveall = false;
                    altblink = moveall;
                    input = Confirm;
                } else {
                    moveall = true;
                }
            } else {
                if ( move_target(input, ch) == true ) {
                    update = true;
                }
            }
            if (update) {
                blink = true;
                update = false;
                recalc_target( shape );
                altblink = moveall;
                update_view(false);
            }
        } else {
            blink = !blink;
        }
        altblink = moveall;
        update_view(false);
    } while (ch != 'q' && input != Confirm);
    timeout(-1);
    blink = true;
    altblink = false;
    if ( input == Confirm ) {
        editshape = shape;
        update_view(false);
        return target_list.size();
    } else {
        target_list.clear();
        target = orig;
        origin = origor;
        target_list.push_back(target);
        blink = false;
        update_view(false);
        return -1;
    }
}



#ifndef has_real_coords
/* stubbing out mapgen functionality */
int editmap::mapgen_preview( real_coords &tc, uimenu &gmenu )
{
    return 0;
}
int editmap::mapgen_retarget ( WINDOW *preview, map *mptr)
{
    return 0;
}
int editmap::apply_mapgen(point coords)
{
    return 0;
}

#else

/* censored */




#endif
