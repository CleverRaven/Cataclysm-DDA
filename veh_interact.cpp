#include <string>
#include "veh_interact.h"
#include "vehicle.h"
#include "keypress.h"
#include "game.h"
#include "output.h"
#include "catacharset.h"
#include "crafting.h"
#include "options.h"
#include "debug.h"

veh_interact::veh_interact ()
{
    cx = 0;
    cy = 0;
    cpart = -1;
    ddx = 0;
    ddy = 0;
    sel_cmd = ' ';
    sel_type=0;
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
    winw3 = FULL_SCREEN_WIDTH - winw1 - winw2 - 2;
    winh3 = FULL_SCREEN_HEIGHT - winh1 - winh2 - 2;
    winh23 = winh2 + winh3 + 1;
    winx1 = winw1;
    winx2 = winw1 + winw2 + 1;
    winy1 = winh1;
    winy2 = winh1 + winh2 + 1;

    // changed FALSE value to 1, to keep w_border from starting at a negative x,y
    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 1;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 1;

    page_size = winh23;

    //               h   w    y     x
    WINDOW *w_border= newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,  -1 + iOffsetY,    -1 + iOffsetX);
    w_grid  = newwin(FULL_SCREEN_HEIGHT -2, FULL_SCREEN_WIDTH-2,  iOffsetY,    iOffsetX);
    w_mode  = newwin(1,  FULL_SCREEN_WIDTH-2, iOffsetY,    iOffsetX);
    w_msg   = newwin(winh1 - 1, FULL_SCREEN_WIDTH-2, 1 + iOffsetY,    iOffsetX);
    w_disp  = newwin(winh2-1, winw1,  winy1 + 1 + iOffsetY, iOffsetX);
    w_parts = newwin(winh2-1, winw2,  winy1 + 1 + iOffsetY, winx1 + 1 + iOffsetX);
    w_stats = newwin(winh3-1, winw12, winy2 + iOffsetY, iOffsetX);
    w_list  = newwin(winh23, winw3, winy1 + 1 + iOffsetY, winx2 + 1 + iOffsetX);

    wborder(w_border, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                      LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

    mvwputch(w_border, 16, 0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_border, 4, 0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_border, 4, FULL_SCREEN_WIDTH-1, c_ltgray, LINE_XOXX); // -|
    mvwputch(w_border, 24, 49, c_ltgray, LINE_XXOX);

    wrefresh(w_border);

    for (int i = 0; i < FULL_SCREEN_HEIGHT; i++)
    {
        mvwputch(w_grid, i, winx2, c_ltgray, i == winy1 || i == winy2-1? LINE_XOXX : LINE_XOXO);
        if (i >= winy1 && i < winy2)
            mvwputch(w_grid, i, winx1, c_ltgray, LINE_XOXO);
    }
    for (int i = 0; i < FULL_SCREEN_WIDTH; i++)
    {
        mvwputch(w_grid, winy1, i, c_ltgray,
                 i == winx1? LINE_OXXX : (i == winx2? LINE_OXXX : LINE_OXOX));
        if (i < winx2)
            mvwputch(w_grid, winy2-1, i, c_ltgray, i == winx1? LINE_XXOX : LINE_OXOX);
    }
    wrefresh(w_grid);

    crafting_inv = gm->crafting_inventory(&gm->u);

    int charges = ((it_tool *) g->itypes["welder"])->charges_per_use;
    int charges_crude = ((it_tool *) g->itypes["welder_crude"])->charges_per_use;
    has_wrench = crafting_inv.has_amount("wrench", 1) ||
        crafting_inv.has_amount("toolset", 1);
    has_hacksaw = crafting_inv.has_amount("hacksaw", 1) ||
        crafting_inv.has_amount("toolset", 1);
    has_welder = (crafting_inv.has_amount("welder", 1) &&
                  crafting_inv.has_charges("welder", charges)) ||
                  (crafting_inv.has_amount("welder_crude", 1) &&
                  crafting_inv.has_charges("welder_crude", charges_crude)) ||
                (crafting_inv.has_amount("toolset", 1) &&
                 crafting_inv.has_charges("toolset", charges/20));
    has_jack = crafting_inv.has_amount("jack", 1);
    has_siphon = crafting_inv.has_amount("hose", 1);

    has_wheel = 0;
    has_wheel |= crafting_inv.has_amount( "wheel", 1 );
    has_wheel |= crafting_inv.has_amount( "wheel_wide", 1 );
    has_wheel |= crafting_inv.has_amount( "wheel_bicycle", 1 );
    has_wheel |= crafting_inv.has_amount( "wheel_motorbike", 1 );
    has_wheel |= crafting_inv.has_amount( "wheel_small", 1 );

    display_stats ();
    display_veh   ();
    move_cursor (0, 0);
    bool finish = false;
    while (!finish)
    {
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (gm, dx, dy, ch);
        if (ch == KEY_ESCAPE || ch == 'q' )
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
                case 'e': do_rename(mval);  break;
                case 's': do_siphon(mval);  break;
                case 'c': do_tirechange(mval); break;
                case 'd': do_drain(mval);  break;
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
    bool valid_target = false;
    bool has_tools = false;
    bool part_free = true;
    bool has_skill = true;

    switch (mode)
    {
    case 'i': // install mode
        valid_target = can_mount.size() > 0 && 0 == veh->tags.count("convertible");
        has_tools = has_wrench && has_welder;
        break;
    case 'r': // repair mode
        valid_target = need_repair.size() > 0 && cpart >= 0;
        has_tools = has_welder;
        break;
    case 'f': // refill mode
        valid_target = ptank >= 0;
        has_tools = has_fuel;
        break;
    case 'o': // remove mode
        valid_target = cpart >= 0 && 0 == veh->tags.count("convertible");
        has_tools = has_wrench && has_hacksaw;
        part_free = parts_here.size() > 1 || (cpart >= 0 && veh->can_unmount(cpart));
        has_skill = g->u.skillLevel("mechanics") >= 2;
        break;
    case 's': // siphon mode
        valid_target = veh->fuel_left("gasoline") > 0;
        has_tools = has_siphon;
        break;
    case 'c': // Change tire
        valid_target = wheel >= 0;
        has_tools = has_wrench && has_jack && has_wheel;
        break;
    case 'd': //drain tank
        valid_target = veh->fuel_left("water") > 0;
        has_tools = has_siphon;
    default:
        return -1;
    }

    if( !valid_target ) {
        return 1;
    }
    if( !has_tools ) {
        return 2;
    }
    if( !part_free ) {
        return 3;
    }
    if( !has_skill ) {
        return 4;
    }
    return 0;
}

void veh_interact::do_install(int reason)
{
    werase (w_msg);
    if (g->u.morale_level() < MIN_MORALE_CRAFT)
    { // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, _("Your morale is too low to construct..."));
        wrefresh (w_msg);
        return;
    }
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, _("Cannot install any part here."));
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, rm_prefix(_("<veh>You need a ")).c_str());
        wprintz(w_msg, has_wrench? c_ltgreen : c_red, _("wrench"));
        wprintz(w_msg, c_ltgray, rm_prefix(_("<veh> and a ")).c_str());
        wprintz(w_msg, has_welder? c_ltgreen : c_red, _("powered welder"));
        wprintz(w_msg, c_ltgray, rm_prefix(_("<veh> to install parts.")).c_str());
        wrefresh (w_msg);
        return;
    default:;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose new part to install here:      "));
    wrefresh (w_mode);
    int pos = 0;
    int engines = 0;
    int dif_eng = 0;
    for (int p = 0; p < veh->parts.size(); p++)
    {
        if (veh->part_flag (p, vpf_engine))
        {
            engines++;
            dif_eng = dif_eng / 2 + 12;
        }
    }
    while (true)
    {
        sel_part = can_mount[pos];
        display_list (pos);
        itype_id itm = vpart_list[sel_part].item;
        bool has_comps = crafting_inv.has_amount(itm, 1);
        bool has_skill = g->u.skillLevel("mechanics") >= vpart_list[sel_part].difficulty;
        bool has_tools = has_welder && has_wrench;
        werase (w_msg);
        mvwprintz(w_msg, 0, 1, c_ltgray, rm_prefix(_("<veh>Needs ")).c_str());
        wprintz(w_msg, has_comps? c_ltgreen : c_red, g->itypes[itm]->name.c_str());
        wprintz(w_msg, c_ltgray, rm_prefix(_("<veh>, a ")).c_str());
        wprintz(w_msg, has_wrench? c_ltgreen : c_red, _("wrench"));
        wprintz(w_msg, c_ltgray, rm_prefix(_("<veh>, a ")).c_str());
        wprintz(w_msg, has_welder? c_ltgreen : c_red, _("powered welder"));
        wprintz(w_msg, c_ltgray, rm_prefix(_("<veh>, and level ")).c_str());
        wprintz(w_msg, has_skill? c_ltgreen : c_red, "%d", vpart_list[sel_part].difficulty);
        wprintz(w_msg, c_ltgray, rm_prefix(_("<veh> skill in mechanics.")).c_str());
        bool eng = vpart_list[sel_part].flags & mfb (vpf_engine);
        bool has_skill2 = !eng || (g->u.skillLevel("mechanics") >= dif_eng);
        if (engines && eng) // already has engine
        {
            wprintz(w_msg, c_ltgray, rm_prefix(_("<veh> You also need level ")).c_str());
            wprintz(w_msg, has_skill2? c_ltgreen : c_red, "%d", dif_eng);
            wprintz(w_msg, c_ltgray, rm_prefix(_("<veh> skill in mechanics to install additional engine.")).c_str());
        }
        wrefresh (w_msg);
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (g, dx, dy, ch);
        if ((ch == '\n' || ch == ' ') && has_comps && has_tools && has_skill && has_skill2)
        {
            //if(itm.is_var_veh_part() && crafting_inv.has_amount(itm, 2);
            sel_cmd = 'i';
            return;
        }
        else
        {
            if (ch == KEY_ESCAPE || ch == 'q' )
            {
                werase (w_list);
                wrefresh (w_list);
                werase (w_msg);
                break;
            }
        }
        if (dy == -1 || dy == 1)
        {
            pos += dy;
            if (pos < 0)
            {
                pos = can_mount.size()-1;
            }
            else if (pos >= can_mount.size())
            {
                pos = 0;
            }
        }
    }
}


void veh_interact::do_repair(int reason)
{
    werase (w_msg);
    if (g->u.morale_level() < MIN_MORALE_CRAFT)
    { // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, _("Your morale is too low to construct..."));
        wrefresh (w_msg);
        return;
    }
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, _("There are no damaged parts here."));
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, _("You need a powered welder to repair."));
        mvwprintz(w_msg, 0, 12, has_welder? c_ltgreen : c_red, _("powered welder"));
        wrefresh (w_msg);
        return;
    default:;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose a part here to repair:"));
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
        bool has_skill = g->u.skillLevel("mechanics") >= dif;
        mvwprintz(w_msg, 0, 1, c_ltgray, _("You need level %d skill in mechanics."), dif);
        mvwprintz(w_msg, 0, 16, has_skill? c_ltgreen : c_red, "%d", dif);
        if (veh->parts[sel_part].hp <= 0)
        {
            itype_id itm = veh->part_info(sel_part).item;
            has_comps = crafting_inv.has_amount(itm, 1);
            mvwprintz(w_msg, 1, 1, c_ltgray, _("You also need a wrench and %s to replace broken one."),
                      g->itypes[itm]->name.c_str());
            mvwprintz(w_msg, 1, 17, has_wrench? c_ltgreen : c_red, _("wrench"));
            mvwprintz(w_msg, 1, 28, has_comps? c_ltgreen : c_red, g->itypes[itm]->name.c_str());
        }
        wrefresh (w_msg);
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (g, dx, dy, ch);
        if ((ch == '\n' || ch == ' ') && has_comps && (veh->parts[sel_part].hp > 0 ||
                                                       has_wrench) && has_skill)
        {
            sel_cmd = 'r';
            return;
        }
        else
            if (ch == KEY_ESCAPE || ch == 'q' )
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
            if(pos >= need_repair.size())
                pos = 0;
            else if(pos < 0)
                pos = need_repair.size() - 1;
        }
    }
}

void veh_interact::do_refill(int reason)
{
    werase (w_msg);
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, _("There's no fuel tank here."));
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, _("You need %s."),
                  ammo_name(veh->part_info(ptank).fuel_type).c_str());
        mvwprintz(w_msg, 0, 10, c_red, ammo_name(veh->part_info(ptank).fuel_type).c_str());
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
        mvwprintz(w_msg, 0, 1, c_ltred, _("Your morale is too low to construct..."));
        wrefresh (w_msg);
        return;
    }
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, _("No parts here."));
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, rm_prefix(_("<veh>You need a ")).c_str());
        wprintz(w_msg, has_wrench? c_ltgreen : c_red, _("wrench"));
        wprintz(w_msg, c_ltgray, rm_prefix(_("<veh> and a ")).c_str());
        wprintz(w_msg, has_hacksaw? c_ltgreen : c_red, _("hacksaw"));
        wprintz(w_msg, c_ltgray, rm_prefix(_("<veh> to remove parts.")).c_str());
        if(wheel) {
            mvwprintz(w_msg, 1, 1, c_ltgray, rm_prefix(_("<veh>To change a wheel you need a ")).c_str());
            wprintz(w_msg, has_wrench? c_ltgreen : c_red, _("wrench"));
            wprintz(w_msg, c_ltgray, rm_prefix(_("<veh> and a ")).c_str());
            wprintz(w_msg, has_jack? c_ltgreen : c_red, _("jack"));
        }
        wrefresh (w_msg);
        return;
    case 3:
        mvwprintz(w_msg, 0, 1, c_ltred,
                  _("You cannot remove mount point while something is attached to it."));
        wrefresh (w_msg);
        return;
    case 4:
        mvwprintz(w_msg, 0, 1, c_ltred, _("You need level 2 mechanics skill to remove parts."));
        wrefresh (w_msg);
        return;
    default:;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose a part here to remove:"));
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
        get_direction (g, dx, dy, ch);
        if (ch == '\n' || ch == ' ')
        {
            sel_cmd = 'o';
            return;
        }
        else
            if (ch == KEY_ESCAPE || ch == 'q' )
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

void veh_interact::do_siphon(int reason)
{
    werase (w_msg);
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, _("The vehicle has no gasoline to siphon."));
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, _("You need a hose to siphon fuel."));
        mvwprintz(w_msg, 0, 12, c_red, _("hose"));
        wrefresh (w_msg);
        return;
    default:;
    }
    sel_cmd = 's';
}

void veh_interact::do_tirechange(int reason)
{
    werase( w_msg );
    switch( reason ) {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, _("There is no wheel to change here."));
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 1, 1, c_ltgray, rm_prefix(_("<veh>To change a wheel you need a ")).c_str());
        wprintz(w_msg, has_wrench? c_ltgreen : c_red, _("wrench"));
        wprintz(w_msg, c_ltgray, rm_prefix(_("<veh> and a ")).c_str());
        wprintz(w_msg, has_jack? c_ltgreen : c_red, _("jack"));
        return;
    default:;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose wheel to use as replacement:      "));
    wrefresh (w_mode);
    int pos = 0;
    while (true)
    {
        bool is_wheel = false;
        sel_part = can_mount[pos];
        switch(sel_part) {
        case vp_wheel:
        case vp_wheel_wide:
        case vp_wheel_bicycle:
        case vp_wheel_motorbike:
        case vp_wheel_small:
            is_wheel = true;
            break;
        default:
            break;
        }
        display_list (pos);
        itype_id itm = vpart_list[sel_part].item;
        bool has_comps = crafting_inv.has_amount(itm, 1);
        bool has_tools = has_jack && has_wrench;
        werase (w_msg);
        wrefresh (w_msg);
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (g, dx, dy, ch);
        if ((ch == '\n' || ch == ' ') && has_comps && has_tools && is_wheel)
        {
            sel_cmd = 'c';
            return;
        }
        else
        {
            if (ch == KEY_ESCAPE || ch == 'q' )
            {
                werase (w_list);
                wrefresh (w_list);
                werase (w_msg);
                break;
            }
        }
        if (dy == -1 || dy == 1)
        {
            pos += dy;
            if (pos < 0)
            {
                pos = can_mount.size()-1;
            }
            else if (pos >= can_mount.size())
            {
                pos = 0;
            }
        }
    }
}

void veh_interact::do_drain(int reason)
{
    werase (w_msg);
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, _("The vehicle has no water to siphon.") );
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, _("You need a hose to siphon water.") );
        mvwprintz(w_msg, 0, 12, c_red, _("hose") );
        wrefresh (w_msg);
        return;
    default:;
    }
    sel_cmd = 'd';
}

void veh_interact::do_rename(int reason)
{
    std::string name = string_input_popup(_("Enter new vehicle name:"), 20);
    (veh->name = name);
    werase(w_stats);
    werase(w_grid);
    display_stats ();
    display_veh   ();
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
    mvwputch (w_disp, cy + 6, cx + 6, cpart >= 0 ? veh->part_color (cpart) : c_black,
              special_symbol(cpart >= 0 ? veh->part_sym (cpart) : ' '));
    cx += dx;
    cy += dy;
    cpart = part_at (cx, cy);
    int vdx = -ddx - cy;
    int vdy = cx - ddy;
    int vx, vy;
    veh->coord_translate (vdx, vdy, vx, vy);
    int vehx = veh->global_x() + vx;
    int vehy = veh->global_y() + vy;
    bool obstruct = g->m.move_cost_ter_furn (vehx, vehy) == 0;
    vehicle *oveh = g->m.veh_at (vehx, vehy);
    if (oveh && oveh != veh)
    {
        obstruct = true;
    }
    nc_color col = cpart >= 0 ? veh->part_color (cpart) : c_black;
    mvwputch (w_disp, cy + 6, cx + 6, obstruct ? red_background(col) : hilite(col),
              special_symbol(cpart >= 0 ? veh->part_sym (cpart) : ' '));
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
    wheel = -1;
    if (cpart >= 0)
    {
        parts_here = veh->internal_parts(cpart);
        parts_here.insert (parts_here.begin(), cpart);
        for (int i = 0; i < parts_here.size(); i++)
        {
            int p = parts_here[i];
            if (veh->parts[p].hp < veh->part_info(p).durability)
            {
                need_repair.push_back (i);
            }
            if (veh->part_flag(p, vpf_fuel_tank) && veh->parts[p].amount < veh->part_info(p).size)
            {
                ptank = p;
            }
            if (veh->part_flag(p, vpf_wheel) && veh->parts[p].amount < veh->part_info(p).size)
            {
                wheel = p;
            }
        }
    }
    has_fuel = ptank >= 0 ? g->pl_refill_vehicle(*veh, ptank, true) : false;
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
        long sym = veh->part_sym (p);
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
    mvwprintz(w_stats, 0, 1, c_ltgray, _("Name: "));
    mvwprintz(w_stats, 0, 7, c_ltgreen, veh->name.c_str());
    if(OPTIONS["USE_METRIC_SPEEDS"] == "km/h") {
        mvwprintz(w_stats, 1, 1, c_ltgray, _("Safe speed:      Km/h"));
        mvwprintz(w_stats, 1, 14, c_ltgreen,"%3d", int(veh->safe_velocity(false) * 0.0161f));
        mvwprintz(w_stats, 2, 1, c_ltgray, _("Top speed:       Km/h"));
        mvwprintz(w_stats, 2, 14, c_ltred, "%3d", int(veh->max_velocity(false) * 0.0161f));
        mvwprintz(w_stats, 3, 1, c_ltgray, _("Accel.:          Kmh/t"));
        mvwprintz(w_stats, 3, 14, c_ltblue,"%3d", int(veh->acceleration(false) * 0.0161f));
    }
    else {
        mvwprintz(w_stats, 1, 1, c_ltgray, _("Safe speed:      mph"));
        mvwprintz(w_stats, 1, 14, c_ltgreen,"%3d", veh->safe_velocity(false) / 100);
        mvwprintz(w_stats, 2, 1, c_ltgray, _("Top speed:       mph"));
        mvwprintz(w_stats, 2, 14, c_ltred, "%3d", veh->max_velocity(false) / 100);
        mvwprintz(w_stats, 3, 1, c_ltgray, _("Accel.:          mph/t"));
        mvwprintz(w_stats, 3, 14, c_ltblue,"%3d", veh->acceleration(false) / 100);
    }
    if (OPTIONS["USE_METRIC_WEIGHTS"] == "kg"){
        mvwprintz(w_stats, 4, 1, c_ltgray, _("Mass:            kg"));
        mvwprintz(w_stats, 4, 12, c_ltblue,"%5d", (int) (veh->total_mass()));
    } else {
        mvwprintz(w_stats, 4, 1, c_ltgray, _("Mass:            lbs"));
        mvwprintz(w_stats, 4, 12, c_ltblue,"%5d", (int) (veh->total_mass() * 2.2));
    }
    mvwprintz(w_stats, 5, 26, c_ltgray, _("K dynamics:        "));
    mvwprintz(w_stats, 5, 37, c_ltblue, "%3d", (int) (veh->k_dynamics() * 100));
    mvwputch (w_stats, 5, 41, c_ltgray, '%');
    mvwprintz(w_stats, 6, 26, c_ltgray, _("K mass:            "));
    mvwprintz(w_stats, 6, 37, c_ltblue, "%3d", (int) (veh->k_mass() * 100));
    mvwputch (w_stats, 6, 41, c_ltgray, '%');
    mvwprintz(w_stats, 5, 1, c_ltgray, _("Wheels: "));
    mvwprintz(w_stats, 5, 11, conf? c_ltgreen : c_ltred, (conf? _("<wheels>enough") : _("<wheels>  lack"))+8);
    mvwprintz(w_stats, 6, 1, c_ltgray,  _("Fuel usage (safe):        "));
    int xfu = 20;
    ammotype ftypes[3] = { "gasoline", "battery", "plasma" };
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
    veh->print_fuel_indicator (w_stats, 0, 26, true, true);
    wrefresh (w_stats);
}

void veh_interact::display_mode (char mode)
{
    werase (w_mode);
    int x = 1;
    if (mode == ' ')
    {
        bool mi = !cant_do('i');
        bool mr = !cant_do('r');
        bool mf = !cant_do('f');
        bool mo = !cant_do('o');
        bool ms = !cant_do('s');
        bool mc = !cant_do('c');
        x += shortcut_print(w_mode, 0, x, mi? c_ltgray : c_dkgray, mi? c_ltgreen : c_green, _("<i>nstall"))+1;
        x += shortcut_print(w_mode, 0, x, mr? c_ltgray : c_dkgray, mr? c_ltgreen : c_green, _("<r>epair"))+1;
        x += shortcut_print(w_mode, 0, x, mf? c_ltgray : c_dkgray, mf? c_ltgreen : c_green, _("re<f>ill"))+1;
        x += shortcut_print(w_mode, 0, x, mo? c_ltgray : c_dkgray, mo? c_ltgreen : c_green, _("rem<o>ve"))+1;
        x += shortcut_print(w_mode, 0, x, ms? c_ltgray : c_dkgray, ms? c_ltgreen : c_green, _("<s>iphon"))+1;
        x += shortcut_print(w_mode, 0, x, ms? c_ltgray : c_dkgray, ms? c_ltgreen : c_green, _("<d>rain water"))+1;
        x += shortcut_print(w_mode, 0, x, mc? c_ltgray : c_dkgray, mc? c_ltgreen : c_green, _("<c>hange tire"))+1;
    }
    x += shortcut_print(w_mode, 0, x, c_ltgray, c_ltgreen, _("r<e>name"))+1;
    std::string backstr = _("<ESC>-back");
    int w = utf8_width(backstr.c_str())-2;
    x = 78-w; // right text align
    shortcut_print(w_mode, 0, x, c_ltgray, c_ltgreen, _("<ESC>-back"));
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
        bool has_skill = g->u.skillLevel("mechanics") >= vpart_list[can_mount[i]].difficulty;
        bool is_wheel = vpart_list[can_mount[i]].flags & mfb(vpf_wheel);
        nc_color col = has_comps && (has_skill || is_wheel) ? c_white : c_dkgray;
        mvwprintz(w_list, y, 3, pos == i? hilite (col) : col, vpart_list[can_mount[i]].name);
        mvwputch (w_list, y, 1,
                  vpart_list[can_mount[i]].color, special_symbol (vpart_list[can_mount[i]].sym));
    }
    wrefresh (w_list);
}

struct candidate_vpart {
    bool in_inventory;
    int mapx;
    int mapy;
    union
    {
        signed char invlet;
        int index;
    };
    item vpart_item;
    candidate_vpart(int x, int y, int i, item vpitem):
        in_inventory(false),mapx(x),mapy(y),index(i) { vpart_item = vpitem; }
    candidate_vpart(char ch, item vpitem):
        in_inventory(true),mapx(-1),mapy(-1),invlet(ch) { vpart_item = vpitem; }
};

// given vpart type, give a choice from inventory items & nearby items.
// not using consume_items in crafting.cpp
// because it got into weird cases, & it doesn't consider
// characteristics like item hp & bigness.
item consume_vpart_item (game *g, vpart_id vpid){
    std::vector<candidate_vpart> candidates;
    const itype_id itid = vpart_list[vpid].item;
    for (int x = g->u.posx - PICKUP_RANGE; x <= g->u.posx + PICKUP_RANGE; x++)
        for (int y = g->u.posy - PICKUP_RANGE; y <= g->u.posy + PICKUP_RANGE; y++)
            for(int i=0; i < g->m.i_at(x,y).size(); i++){
                item* ith_item = &(g->m.i_at(x,y)[i]);
                if (ith_item->type->id == itid)
                    candidates.push_back (candidate_vpart(x,y,i,*ith_item));
            }

    std::vector<item*> cand_from_inv = g->u.inv.all_items_by_type(itid);
    for (int i=0; i < cand_from_inv.size(); i++){
        item* ith_item = cand_from_inv[i];
        if (ith_item->type->id  == itid)
            candidates.push_back (candidate_vpart(ith_item->invlet,*ith_item));
    }
    if (g->u.weapon.type->id == itid) {
        candidates.push_back (candidate_vpart(-1,g->u.weapon));
    }

    // bug?
    if(candidates.size() == 0){
        debugmsg("part not found");
        return item();
    }

    int selection;
    // no choice?
    if(candidates.size() == 1) {
        selection = 0;
    } else {
        // popup menu!?
        std::vector<std::string> options;
        for(int i=0;i<candidates.size(); i++){
            if(candidates[i].in_inventory){
                if (candidates[i].invlet == -1)
                    options.push_back(candidates[i].vpart_item.tname() + _(" (wielded)"));
                else
                    options.push_back(candidates[i].vpart_item.tname());
            }
            else { //nearby.
                options.push_back(candidates[i].vpart_item.tname() + _(" (nearby)"));
            }
        }
        selection = menu_vec(false, _("Use which gizmo?"), options);
        selection -= 1;
    }
    //remove item from inventory. or map.
    if(candidates[selection].in_inventory){
        if(candidates[selection].invlet == -1) //weapon
            g->u.remove_weapon();
        else //non-weapon inventory
            g->u.inv.remove_item_by_letter(candidates[selection].invlet);
    } else { //map.
        int x = candidates[selection].mapx;
        int y = candidates[selection].mapy;
        int i = candidates[selection].index;
        g->m.i_rem(x,y,i);
    }
    return candidates[selection].vpart_item;
    //item ret = candidates[selection].vpart_item;
    //return ret;
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
    int type = g->u.activity.values[7];
    std::vector<component> tools;
    int welder_charges = ((it_tool *) g->itypes["welder"])->charges_per_use;
    int welder_crude_charges = ((it_tool *) g->itypes["welder_crude"])->charges_per_use;
    itype_id itm;
    int partnum;
    item used_item;
    bool broken;
    int replaced_wheel;
    std::vector<int> parts;
    int dd = 2;
    switch (cmd)
    {
    case 'i':
        partnum = veh->install_part (dx, dy, (vpart_id) part);
        if(partnum < 0)
            debugmsg ("complete_vehicle install part fails dx=%d dy=%d id=%d", dx, dy, part);
        used_item = consume_vpart_item (g, (vpart_id) part);
        veh->get_part_properties_from_item(g, partnum, used_item); //transfer damage, etc.
        tools.push_back(component("welder", welder_charges));
        tools.push_back(component("welder_crude", welder_crude_charges));
        tools.push_back(component("toolset", welder_charges/20));
        g->consume_tools(&g->u, tools, true);

        if ( part == vp_head_light ) {
            // Need map-relative coordinates to compare to output of look_around.
            int gx, gy;
            // Need to call coord_translate() directly since it's a new part.
            veh->coord_translate(dx, dy, gx, gy);
            // Stash offset and set it to the location of the part so look_around will start there.
            int px = g->u.view_offset_x;
            int py = g->u.view_offset_y;
            g->u.view_offset_x = veh->global_x() + gx - g->u.posx;
            g->u.view_offset_y = veh->global_y() + gy - g->u.posy;
            popup("Choose a facing direction for the new headlight.");
            point headlight_target = g->look_around();
            // Restore previous view offsets.
            g->u.view_offset_x = px;
            g->u.view_offset_y = py;

            int delta_x = headlight_target.x - (veh->global_x() + gx);
            int delta_y = headlight_target.y - (veh->global_y() + gy);

            const double PI = 3.14159265358979f;
            int dir = (atan2(delta_y, delta_x) * 180.0 / PI);
            dir -= veh->face.dir();
            while(dir < 0) dir += 360;
            while(dir > 360) dir -= 360;

            veh->parts[partnum].direction = dir;
        }

        g->add_msg (_("You install a %s into the %s."),
                    vpart_list[part].name, veh->name.c_str());
        g->u.practice (g->turn, "mechanics", vpart_list[part].difficulty * 5 + 20);
        break;
    case 'r':
        if (veh->parts[part].hp <= 0)
        {
            used_item = consume_vpart_item (g, veh->parts[part].id);
            tools.push_back(component("wrench", -1));
            g->consume_tools(&g->u, tools, true);
            tools.clear();
            dd = 0;
            veh->insides_dirty = true;
        }
        tools.push_back(component("welder", welder_charges));
        tools.push_back(component("welder_crude", welder_crude_charges));
        tools.push_back(component("toolset", welder_charges/20));
        g->consume_tools(&g->u, tools, true);
        veh->parts[part].hp = veh->part_info(part).durability;
        g->add_msg (_("You repair the %s's %s."),
                    veh->name.c_str(), veh->part_info(part).name);
        g->u.practice (g->turn, "mechanics", (vpart_list[part].difficulty + dd) * 5 + 20);
        break;
    case 'f':
        if (!g->pl_refill_vehicle(*veh, part, true))
            debugmsg ("complete_vehicle refill broken");
        g->pl_refill_vehicle(*veh, part);
        break;
    case 'o':
        // Dump contents of part at player's feet, if any.
        for (int i = 0; i < veh->parts[part].items.size(); i++)
            g->m.add_item_or_charges (g->u.posx, g->u.posy, veh->parts[part].items[i]);
        veh->parts[part].items.clear();

        broken = veh->parts[part].hp <= 0;
        if (!broken) {
            used_item = veh->item_from_part( part );
            g->m.add_item_or_charges(g->u.posx, g->u.posy, used_item);
            if(type!=SEL_JACK) // Changing tires won't make you a car mechanic
                g->u.practice (g->turn, "mechanics", 2 * 5 + 20);
        }
        if (veh->parts.size() < 2)
        {
            g->add_msg (_("You completely dismantle %s."), veh->name.c_str());
            g->u.activity.type = ACT_NULL;
            g->m.destroy_vehicle (veh);
        }
        else
        {
            g->add_msg (_("You remove %s%s from %s."), broken? rm_prefix(_("<veh>broken ")).c_str() : "",
                        veh->part_info(part).name, veh->name.c_str());
            veh->remove_part (part);
        }
        break;
    case 's':
        g->u.siphon( g, veh, "gasoline" );
        break;
    case 'c':
        parts = veh->parts_at_relative( dx, dy );
        if( parts.size() ) {
            item removed_wheel;
            replaced_wheel = veh->part_with_feature( parts[0], vpf_wheel, false );
            broken = veh->parts[replaced_wheel].hp <= 0;
            if( replaced_wheel != -1 ) {
                removed_wheel = veh->item_from_part( replaced_wheel );
                veh->remove_part( replaced_wheel );
                g->add_msg( _("You replace one of the %s's tires with %s."),
                            veh->name.c_str(), vpart_list[part].name );
            } else {
                debugmsg( "no wheel to remove when changing wheels." );
                return;
            }
            partnum = veh->install_part( dx, dy, (vpart_id) part );
            if( partnum < 0 )
                debugmsg ("complete_vehicle tire change fails dx=%d dy=%d id=%d", dx, dy, part);
            used_item = consume_vpart_item( g, (vpart_id) part );
            veh->get_part_properties_from_item( g, partnum, used_item ); //transfer damage, etc.
            // Place the removed wheel on the map last so consume_vpart_item() doesn't pick it.
            if ( !broken ) {
                g->m.add_item_or_charges( g->u.posx, g->u.posy, removed_wheel );
            }
        }
        break;
    case 'd':
        g->u.siphon( g, veh, "water" );
        break;
    default:;
    }
}
