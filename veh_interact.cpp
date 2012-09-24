#include <string>
#include "veh_interact.h"
#include "vehicle.h"
#include "keypress.h"
#include "game.h"
#include "output.h"
#include "crafting.h"
#include "options.h"


veh_interact::veh_interact ()
{
    cx = 0;
    cy = 0;
    cpart = -1;
    ddx = 0;
    ddy = 0;
    sel_cmd = ' ';
}

void veh_interact::exec (game *gm, vehicle *v, int x, int y)
{
    g = gm;
    veh = v;
    ex = x;
    ey = y;
    //        x1      x2
    // y1 ----+------+--
    //        |      |
    // y2 ----+------+
    //               |
    //               |
    winw1 = 12;
    winw2 = 35;
    winh1 = 3;
    winh2 = 12;
    winw12 = winw1 + winw2 + 1;
    winw3 = 80 - winw1 - winw2 - 2;
    winh3 = 25 - winh1 - winh2 - 2;
    winh23 = winh2 + winh3 + 1;
    winx1 = winw1;
    winx2 = winw1 + winw2 + 1;
    winy1 = winh1;
    winy2 = winh1 + winh2 + 1;

    page_size = winh23;
    //               h   w    y     x
    w_grid  = newwin(25, 80,  0,    0);
    w_mode  = newwin(1,  80, 0,    0);
    w_msg   = newwin(winh1 - 1, 80, 1,    0);
    w_disp  = newwin(winh2, winw1,  winy1 + 1, 0);
    w_parts = newwin(winh2, winw2,  winy1 + 1, winx1 + 1);
    w_stats = newwin(winh3, winw12, winy2 + 1, 0);
    w_list  = newwin(winh23, winw3, winy1 + 1, winx2 + 1);

    for (int i = 0; i < 25; i++)
    {
        mvwputch(w_grid, i, winx2, c_ltgray, i == winy1 || i == winy2? LINE_XOXX : LINE_XOXO);
        if (i >= winy1 && i < winy2)
            mvwputch(w_grid, i, winx1, c_ltgray, LINE_XOXO);
    }
    for (int i = 0; i < 80; i++)
    {
        mvwputch(w_grid, winy1, i, c_ltgray, i == winx1? LINE_OXXX : (i == winx2? LINE_OXXX : LINE_OXOX));
        if (i < winx2)
            mvwputch(w_grid, winy2, i, c_ltgray, i == winx1? LINE_XXOX : LINE_OXOX);
    }
    wrefresh(w_grid);

    crafting_inv.form_from_map(g, point(g->u.posx, g->u.posy), PICKUP_RANGE);
    crafting_inv += g->u.inv;
    crafting_inv += g->u.weapon;
    if (g->u.has_bionic(bio_tools)) 
    {
        item tools(g->itypes[itm_toolset], g->turn);
        tools.charges = g->u.power_level;
        crafting_inv += tools;
    }
    int charges = ((it_tool *) g->itypes[itm_welder])->charges_per_use;
    has_wrench = crafting_inv.has_amount(itm_wrench, 1) ||
                 crafting_inv.has_amount(itm_toolset, 1);
    has_hacksaw = crafting_inv.has_amount(itm_hacksaw, 1) ||
                  crafting_inv.has_amount(itm_toolset, 1);
    has_welder = (crafting_inv.has_amount(itm_welder, 1) &&
                  crafting_inv.has_charges(itm_welder, charges)) ||
                 (crafting_inv.has_amount(itm_toolset, 1) &&
                 crafting_inv.has_charges(itm_toolset, charges/5));
        
    display_stats ();
    display_veh   ();
    move_cursor (0, 0);
    bool finish = false;
    while (!finish)
    {
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if (ch == KEY_ESCAPE)
            finish = true;
        else
        if (dx != -2 && (dx || dy) &&
            cx + dx >= -6 && cx + dx < 6 &&
            cy + dy >= -6 && cy + dy < 6)
            move_cursor(dx, dy);
        else
        {
            int mval = cant_do(ch);
            display_mode (ch);
            switch (ch)
            {
            case 'i': do_install(mval); break;
            case 'r': do_repair(mval);  break;
            case 'f': do_refill(mval);  break;
            case 'o': do_remove(mval);  break;
            default:;
            }
            if (sel_cmd != ' ')
                finish = true;
            display_mode (' ');
        }
    }
    werase(w_grid);
    werase(w_mode);
    werase(w_msg);
    werase(w_disp);
    werase(w_parts);
    werase(w_stats);
    werase(w_list);
    delwin(w_grid);
    delwin(w_mode);
    delwin(w_msg);
    delwin(w_disp);
    delwin(w_parts);
    delwin(w_stats);
    delwin(w_list);
    erase();
}

int veh_interact::cant_do (char mode)
{
    switch (mode)
    {
    case 'i': // install mode
        return can_mount.size() > 0? (!has_wrench || !has_welder? 2 : 0) : 1;
    case 'r': // repair mode
        return need_repair.size() > 0 && cpart >= 0? (!has_welder? 2 : 0) : 1;
    case 'f': // refill mode
        return ptank >= 0? (!has_fuel? 2 : 0) : 1;
    case 'o': // remove mode
        return cpart < 0? 1 : 
               (parts_here.size() < 2 && !veh->can_unmount(cpart)? 2 : 
               (!has_wrench || !has_hacksaw? 3 :
               (g->u.sklevel[sk_mechanics] < 2 ? 4 : 0)));
    default:
        return -1;
    }
}

void veh_interact::do_install(int reason)
{
    werase (w_msg);
    if (g->u.morale_level() < MIN_MORALE_CRAFT) 
    { // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, "Your morale is too low to construct...");
        wrefresh (w_msg);
        return;
    }
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, "Cannot install any part here.");
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, "You need a wrench and a powered welder to install parts.");
        mvwprintz(w_msg, 0, 12, has_wrench? c_ltgreen : c_red, "wrench");
        mvwprintz(w_msg, 0, 25, has_welder? c_ltgreen : c_red, "powered welder");
        wrefresh (w_msg);
        return;
    default:;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, "Choose new part to install here:");
    wrefresh (w_mode);
    int pos = 0;
    int engines = 0;
    int dif_eng = 0;
    for (int p = 0; p < veh->parts.size(); p++)
        if (veh->part_flag (p, vpf_engine))
        {
            engines++;
            dif_eng = dif_eng / 2 + 12;
        }
    while (true)
    {
        sel_part = can_mount[pos];
        display_list (pos);
        itype_id itm = vpart_list[sel_part].item;
        bool has_comps = crafting_inv.has_amount(itm, 1);
        bool has_skill = g->u.sklevel[sk_mechanics] >= vpart_list[sel_part].difficulty;
        werase (w_msg);
        int slen = g->itypes[itm]->name.length();
        mvwprintz(w_msg, 0, 1, c_ltgray, "Needs %s and level %d skill in mechanics.", 
                  g->itypes[itm]->name.c_str(), vpart_list[sel_part].difficulty);
        mvwprintz(w_msg, 0, 7, has_comps? c_ltgreen : c_red, g->itypes[itm]->name.c_str());
        mvwprintz(w_msg, 0, 18+slen, has_skill? c_ltgreen : c_red, "%d", vpart_list[sel_part].difficulty);
        bool eng = vpart_list[sel_part].flags & mfb (vpf_engine);
        bool has_skill2 = !eng || (g->u.sklevel[sk_mechanics] >= dif_eng);
        if (engines && eng) // already has engine
        {
            mvwprintz(w_msg, 1, 1, c_ltgray, 
                      "You also need level %d skill in mechanics to install additional engine.", 
                      dif_eng);
            mvwprintz(w_msg, 1, 21, has_skill2? c_ltgreen : c_red, "%d", dif_eng);
        }
        wrefresh (w_msg);
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if ((ch == '\n' || ch == ' ') && has_comps && has_skill && has_skill2)
        {
            sel_cmd = 'i';
            return;
        }
        else
        if (ch == KEY_ESCAPE)
        {
            werase (w_list);
            wrefresh (w_list);
            werase (w_msg);
            break;
        }
        if (dy == -1 || dy == 1)
        {
            pos += dy;
            if (pos < 0)
                pos = can_mount.size()-1;
            else
            if (pos >= can_mount.size())
                pos = 0;
        }
    }
}

void veh_interact::do_repair(int reason)
{
    werase (w_msg);
    if (g->u.morale_level() < MIN_MORALE_CRAFT) 
    { // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, "Your morale is too low to construct...");
        wrefresh (w_msg);
        return;
    }
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, "There are no damaged parts here.");
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, "You need a powered welder to repair.");
        mvwprintz(w_msg, 0, 12, has_welder? c_ltgreen : c_red, "powered welder");
        wrefresh (w_msg);
        return;
    default:;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, "Choose a part here to repair:");
    wrefresh (w_mode);
    int pos = 0;
    while (true)
    {
        sel_part = parts_here[need_repair[pos]];
        werase (w_parts);
        veh->print_part_desc (w_parts, 0, winw2, cpart, need_repair[pos]);
        wrefresh (w_parts);
        werase (w_msg);
        bool has_comps = true;
        int dif = veh->part_info(sel_part).difficulty + (veh->parts[sel_part].hp <= 0? 0 : 2);
        bool has_skill = dif <= g->u.sklevel[sk_mechanics];
        mvwprintz(w_msg, 0, 1, c_ltgray, "You need level %d skill in mechanics.", dif);
        mvwprintz(w_msg, 0, 16, has_skill? c_ltgreen : c_red, "%d", dif);
        if (veh->parts[sel_part].hp <= 0)
        {
            itype_id itm = veh->part_info(sel_part).item;
            has_comps = crafting_inv.has_amount(itm, 1);
            mvwprintz(w_msg, 1, 1, c_ltgray, "You also need a wrench and %s to replace broken one.", 
                    g->itypes[itm]->name.c_str());
            mvwprintz(w_msg, 1, 17, has_wrench? c_ltgreen : c_red, "wrench");
            mvwprintz(w_msg, 1, 28, has_comps? c_ltgreen : c_red, g->itypes[itm]->name.c_str());            
        }
        wrefresh (w_msg);
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if ((ch == '\n' || ch == ' ') && has_comps && (veh->parts[sel_part].hp > 0 || has_wrench) && has_skill)
        {
            sel_cmd = 'r';
            return;
        }
        else
        if (ch == KEY_ESCAPE)
        {
            werase (w_parts);
            veh->print_part_desc (w_parts, 0, winw2, cpart, -1);
            wrefresh (w_parts);
            werase (w_msg);
            break;
        }
        if (dy == -1 || dy == 1)
        {
            pos += dy;
            if (pos < 0)
                pos = need_repair.size()-1;
            else
            if (pos >= need_repair.size())
                pos = 0;
        }
    }
}

void veh_interact::do_refill(int reason)
{
    werase (w_msg);
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, "There's no fuel tank here.");
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, "You need %s.", veh->fuel_name(veh->part_info(ptank).fuel_type).c_str());
        mvwprintz(w_msg, 0, 10, c_red, veh->fuel_name(veh->part_info(ptank).fuel_type).c_str());
        wrefresh (w_msg);
        return;
    default:;
    }
    sel_cmd = 'f';
    sel_part = ptank;
}

void veh_interact::do_remove(int reason)
{
    werase (w_msg);
    if (g->u.morale_level() < MIN_MORALE_CRAFT) 
    { // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, "Your morale is too low to construct...");
        wrefresh (w_msg);
        return;
    }
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, "No parts here.");
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltred, "You cannot remove mount point while something is attached to it.");
        wrefresh (w_msg);
        return;
    case 3:
        mvwprintz(w_msg, 0, 1, c_ltgray, "You need a wrench and a hack saw to remove parts.");
        mvwprintz(w_msg, 0, 12, has_wrench? c_ltgreen : c_red, "wrench");
        mvwprintz(w_msg, 0, 25, has_hacksaw? c_ltgreen : c_red, "hack saw");
        wrefresh (w_msg);
        return;
    case 4:
        mvwprintz(w_msg, 0, 1, c_ltred, "You need level 2 mechanics skill to remove parts.");
        wrefresh (w_msg);
        return;
    default:;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, "Choose a part here to remove:");
    wrefresh (w_mode);
    int first = parts_here.size() > 1? 1 : 0;
    int pos = first;
    while (true)
    {
        sel_part = parts_here[pos];
        werase (w_parts);
        veh->print_part_desc (w_parts, 0, winw2, cpart, pos);
        wrefresh (w_parts);
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if (ch == '\n' || ch == ' ')
        {
            sel_cmd = 'o';
            return;
        }
        else
        if (ch == KEY_ESCAPE)
        {
            werase (w_parts);
            veh->print_part_desc (w_parts, 0, winw2, cpart, -1);
            wrefresh (w_parts);
            werase (w_msg);
            break;
        }
        if (dy == -1 || dy == 1)
        {
            pos += dy;
            if (pos < first)
                pos = parts_here.size()-1;
            else
            if (pos >= parts_here.size())
                pos = first;
        }
    }
}

int veh_interact::part_at (int dx, int dy)
{
    int vdx = -ddx - dy;
    int vdy = dx - ddy;
    for (int ep = 0; ep < veh->external_parts.size(); ep++)
    {
        int p = veh->external_parts[ep];
        if (veh->parts[p].mount_dx == vdx && veh->parts[p].mount_dy == vdy)
            return p;
    }
    return -1;
}

void veh_interact::move_cursor (int dx, int dy)
{
    mvwputch (w_disp, cy+6, cx+6, cpart >= 0? veh->part_color (cpart) : c_black, special_symbol(cpart >= 0? veh->part_sym (cpart) : ' '));
    cx += dx;
    cy += dy;
    cpart = part_at (cx, cy);
    int vdx = -ddx - cy;
    int vdy = cx - ddy;
    int vx, vy;
    veh->coord_translate (vdx, vdy, vx, vy);
    int vehx = veh->global_x() + vx;
    int vehy = veh->global_y() + vy;
    bool obstruct = g->m.move_cost_ter_only (vehx, vehy) == 0;
    vehicle *oveh = g->m.veh_at (vehx, vehy);
    if (oveh && oveh != veh)
        obstruct = true;
    nc_color col = cpart >= 0? veh->part_color (cpart) : c_black;
    mvwputch (w_disp, cy+6, cx+6, obstruct? red_background(col) : hilite(col),
                      special_symbol(cpart >= 0? veh->part_sym (cpart) : ' '));
    wrefresh (w_disp);
    werase (w_parts);
    veh->print_part_desc (w_parts, 0, winw2, cpart, -1);
    wrefresh (w_parts);

    can_mount.clear();
    has_mats.clear();
    if (!obstruct)
        for (int i = 1; i < num_vparts; i++)
        {
            if (veh->can_mount (vdx, vdy, (vpart_id) i))
                can_mount.push_back (i);
        }
    need_repair.clear();
    parts_here.clear();
    ptank = -1;
    if (cpart >= 0)
    {
        parts_here = veh->internal_parts(cpart);
        parts_here.insert (parts_here.begin(), cpart);
        for (int i = 0; i < parts_here.size(); i++)
        {
            int p = parts_here[i];
            if (veh->parts[p].hp < veh->part_info(p).durability)
                need_repair.push_back (i);
            if (veh->part_flag(p, vpf_fuel_tank) && veh->parts[p].amount < veh->part_info(p).size)
                ptank = p;
        }
    }
    has_fuel = ptank >= 0? g->pl_refill_vehicle(*veh, ptank, true) : false;
    werase (w_msg);
    wrefresh (w_msg);
    display_mode (' ');
}

void veh_interact::display_veh ()
{
    int x1 = 12, y1 = 12, x2 = -12, y2 = -12;
    for (int ep = 0; ep < veh->external_parts.size(); ep++)
    {
        int p = veh->external_parts[ep];
        if (veh->parts[p].mount_dx < x1)
            x1 = veh->parts[p].mount_dx;
        if (veh->parts[p].mount_dy < y1)
            y1 = veh->parts[p].mount_dy;
        if (veh->parts[p].mount_dx > x2)
            x2 = veh->parts[p].mount_dx;
        if (veh->parts[p].mount_dy > y2)
            y2 = veh->parts[p].mount_dy;
    }
    ddx = 0;
    ddy = 0;
    if (x2 - x1 < 11) { x1--; x2++; }
    if (y2 - y1 < 11 ) { y1--; y2++; }
    if (x1 < -5)
        ddx = -5 - x1;
    else
    if (x2 > 6)
        ddx = 6 - x2;
    if (y1 < -6)
        ddy = -6 - y1;
    else
    if (y2 > 5)
        ddy = 5 - y2;

    for (int ep = 0; ep < veh->external_parts.size(); ep++)
    {
        int p = veh->external_parts[ep];
        char sym = veh->part_sym (p);
        nc_color col = veh->part_color (p);
        int y = -(veh->parts[p].mount_dx + ddx);
        int x = veh->parts[p].mount_dy + ddy;
        mvwputch (w_disp, 6+y, 6+x, cx == x && cy == y? hilite(col) : col, special_symbol(sym));
        if (cx == x && cy == y)
            cpart = p;
    }
    wrefresh (w_disp);
}

void veh_interact::display_stats ()
{
    bool conf = veh->valid_wheel_config();
    mvwprintz(w_stats, 0, 1, c_ltgray, "Name: ");
    mvwprintz(w_stats, 0, 7, c_ltgreen, veh->name.c_str());
    if(OPTIONS[OPT_USE_METRIC_SYS]) {
     mvwprintz(w_stats, 1, 1, c_ltgray, "Safe speed:      Km/h");
     mvwprintz(w_stats, 1, 14, c_ltgreen,"%3d", int(veh->safe_velocity(false) * 0.0161f));
     mvwprintz(w_stats, 2, 1, c_ltgray, "Top speed:       Km/h");
     mvwprintz(w_stats, 2, 14, c_ltred, "%3d", int(veh->max_velocity(false) * 0.0161f));
     mvwprintz(w_stats, 3, 1, c_ltgray, "Accel.:          Kmh/t");
     mvwprintz(w_stats, 3, 14, c_ltblue,"%3d", int(veh->acceleration(false) * 0.0161f));
    }
    else {
     mvwprintz(w_stats, 1, 1, c_ltgray, "Safe speed:      mph");
     mvwprintz(w_stats, 1, 14, c_ltgreen,"%3d", veh->safe_velocity(false) / 100);
     mvwprintz(w_stats, 2, 1, c_ltgray, "Top speed:       mph");
     mvwprintz(w_stats, 2, 14, c_ltred, "%3d", veh->max_velocity(false) / 100);
     mvwprintz(w_stats, 3, 1, c_ltgray, "Accel.:          mph/t");
     mvwprintz(w_stats, 3, 14, c_ltblue,"%3d", veh->acceleration(false) / 100);
    }
    mvwprintz(w_stats, 4, 1, c_ltgray, "Mass:            kg");
    mvwprintz(w_stats, 4, 12, c_ltblue,"%5d", (int) (veh->total_mass() / 4 * 0.45));
    mvwprintz(w_stats, 1, 26, c_ltgray, "K dynamics:        ");
    mvwprintz(w_stats, 1, 37, c_ltblue, "%3d", (int) (veh->k_dynamics() * 100));
    mvwputch (w_stats, 1, 41, c_ltgray, '%');
    mvwprintz(w_stats, 2, 26, c_ltgray, "K mass:            ");
    mvwprintz(w_stats, 2, 37, c_ltblue, "%3d", (int) (veh->k_mass() * 100));
    mvwputch (w_stats, 2, 41, c_ltgray, '%');
    mvwprintz(w_stats, 5, 1, c_ltgray, "Wheels: ");
    mvwprintz(w_stats, 5, 11, conf? c_ltgreen : c_ltred, conf? "enough" : "  lack");
    mvwprintz(w_stats, 6, 1, c_ltgray,  "Fuel usage (safe):        ");
    int xfu = 20;
    int ftypes[3] = { AT_GAS, AT_BATT, AT_PLASMA };
    nc_color fcs[3] = { c_ltred, c_yellow, c_ltblue };
    bool first = true;
    for (int i = 0; i < 3; i++)
    {
        int fu = veh->basic_consumption (ftypes[i]);
        if (fu > 0)
        {
            fu = fu / 100;
            if (fu < 1)
                fu = 1;
            if (!first)
                mvwprintz(w_stats, 6, xfu++, c_ltgray, "/");
            mvwprintz(w_stats, 6, xfu++, fcs[i], "%d", fu);
            if (fu > 9) xfu++;
            if (fu > 99) xfu++;
            first = false;
        }
    }
    veh->print_fuel_indicator (w_stats, 0, 42);
    wrefresh (w_stats);
}

void veh_interact::display_mode (char mode)
{
    werase (w_mode);
    if (mode == ' ')
    {
        bool mi = !cant_do('i');
        bool mr = !cant_do('r');
        bool mf = !cant_do('f');
        bool mo = !cant_do('o');
        mvwprintz(w_mode, 0, 1, mi? c_ltgray : c_dkgray, "install");
        mvwputch (w_mode, 0, 1, mi? c_ltgreen : c_green, 'i');
        mvwprintz(w_mode, 0, 9, mr? c_ltgray : c_dkgray, "repair");
        mvwputch (w_mode, 0, 9, mr? c_ltgreen : c_green, 'r');
        mvwprintz(w_mode, 0, 16, mf? c_ltgray : c_dkgray, "refill");
        mvwputch (w_mode, 0, 18, mf? c_ltgreen : c_green, 'f');
        mvwprintz(w_mode, 0, 23, mo? c_ltgray : c_dkgray, "remove");
        mvwputch (w_mode, 0, 26, mo? c_ltgreen : c_green, 'o');
    }
    mvwprintz(w_mode, 0, 71, c_ltgreen, "ESC");
    mvwprintz(w_mode, 0, 74, c_ltgray, "-back");
    wrefresh (w_mode);
}

void veh_interact::display_list (int pos)
{
    werase (w_list);
    int page = pos / page_size;
    for (int i = page * page_size; i < (page + 1) * page_size && i < can_mount.size(); i++)
    {
        int y = i - page * page_size;
        itype_id itm = vpart_list[can_mount[i]].item;
        bool has_comps = crafting_inv.has_amount(itm, 1);
        bool has_skill = g->u.sklevel[sk_mechanics] >= vpart_list[can_mount[i]].difficulty;
        nc_color col = has_comps && has_skill? c_white : c_dkgray;
        mvwprintz(w_list, y, 3, pos == i? hilite (col) : col, vpart_list[can_mount[i]].name);
        mvwputch (w_list, y, 1, 
                  vpart_list[can_mount[i]].color, special_symbol (vpart_list[can_mount[i]].sym));
    }
    wrefresh (w_list);
}

void complete_vehicle (game *g)
{
    if (g->u.activity.values.size() < 7)
    {
        debugmsg ("Invalid activity ACT_VEHICLE values:%d", g->u.activity.values.size());
        return;
    }
    vehicle *veh = g->m.veh_at (g->u.activity.values[0], g->u.activity.values[1]);
    if (!veh)
    {
        debugmsg ("Activity ACT_VEHICLE: vehicle not found");
        return;
    }
    char cmd = (char) g->u.activity.index;
    int dx = g->u.activity.values[4];
    int dy = g->u.activity.values[5];
    int part = g->u.activity.values[6];
    std::vector<component> comps;
    std::vector<component> tools;
    int welder_charges = ((it_tool *) g->itypes[itm_welder])->charges_per_use;
    itype_id itm;
    bool broken;

    int dd = 2;
    switch (cmd)
    {
    case 'i':
        if (veh->install_part (dx, dy, (vpart_id) part) < 0)
            debugmsg ("complete_vehicle install part fails dx=%d dy=%d id=%d", dx, dy, part);
        comps.push_back(component(vpart_list[part].item, 1));
        consume_items(g, comps);
        tools.push_back(component(itm_welder, welder_charges));
        tools.push_back(component(itm_toolset, welder_charges/5));
        consume_tools(g, tools);
        g->add_msg ("You install a %s into the %s.", 
                   vpart_list[part].name, veh->name.c_str());
        g->u.practice (sk_mechanics, vpart_list[part].difficulty * 5 + 20);
        break;
    case 'r':
        if (veh->parts[part].hp <= 0)
        {
            comps.push_back(component(veh->part_info(part).item, 1));
            consume_items(g, comps);
            tools.push_back(component(itm_wrench, 1));
            consume_tools(g, tools);
            tools.clear();
            dd = 0;
            veh->insides_dirty = true;
        }
        tools.push_back(component(itm_welder, welder_charges));
        tools.push_back(component(itm_toolset, welder_charges/5));
        consume_tools(g, tools);
        veh->parts[part].hp = veh->part_info(part).durability;
        g->add_msg ("You repair the %s's %s.", 
                    veh->name.c_str(), veh->part_info(part).name);
        g->u.practice (sk_mechanics, (vpart_list[part].difficulty + dd) * 5 + 20);
        break;
    case 'f':
        if (!g->pl_refill_vehicle(*veh, part, true))
            debugmsg ("complete_vehicle refill broken");
        g->pl_refill_vehicle(*veh, part);        
        break;
    case 'o':
        for (int i = 0; i < veh->parts[part].items.size(); i++)
            g->m.add_item (g->u.posx, g->u.posy, veh->parts[part].items[i]);
        veh->parts[part].items.clear();
        itm = veh->part_info(part).item;
        broken = veh->parts[part].hp <= 0;
        if (veh->parts.size() < 2)
        {
            g->add_msg ("You completely dismantle %s.", veh->name.c_str());
            g->u.activity.type = ACT_NULL;
            g->m.destroy_vehicle (veh);
        }
        else
        {
            g->add_msg ("You remove %s%s from %s.", broken? "broken " : "",
                        veh->part_info(part).name, veh->name.c_str());
            veh->remove_part (part);
        }
        if (!broken)
            g->m.add_item (g->u.posx, g->u.posy, g->itypes[itm], g->turn);
        g->u.practice (sk_mechanics, 2 * 5 + 20);
        break;
    default:;
        
    }
}



