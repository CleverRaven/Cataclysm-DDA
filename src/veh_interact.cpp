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
#include <cmath>

#ifdef _MSC_VER
#include <math.h>
#define ISNAN _isnan
#else
#define ISNAN std::isnan
#endif

/**
 * Creates a blank veh_interact window.
 */
veh_interact::veh_interact ()
{
    cpart = -1;
    ddx = 0;
    ddy = 0;
    sel_cmd = ' ';
    sel_type = 0;
    sel_vpart_info = NULL;
    sel_vehicle_part = NULL;

    totalDurabilityColor = c_green;
    worstDurabilityColor = c_green;
    durabilityPercent = 100;
    mostDamagedPart = -1;
}

/**
 * Creates a veh_interact window based on the given parameters.
 * @param v The vehicle the player is interacting with.
 * @param x The x-coordinate of the square the player is 'e'xamining.
 * @param y The y-coordinate of the square the player is 'e'xamining.
 */
void veh_interact::exec (game *gm, vehicle *v, int x, int y)
{
    veh = v;
    countDurability();

    crafting_inv = g->crafting_inventory(&g->u);

    int charges = static_cast<it_tool *>(itypes["welder"])->charges_per_use;
    int charges_crude = static_cast<it_tool *>(itypes["welder_crude"])->charges_per_use;
    has_wrench = crafting_inv.has_amount("wrench", 1) ||
                 crafting_inv.has_amount("toolset", 1);
    has_hacksaw = crafting_inv.has_amount("hacksaw", 1) ||
                  crafting_inv.has_amount("toolset", 1);
    has_welder = (crafting_inv.has_amount("welder", 1) &&
                  crafting_inv.has_charges("welder", charges)) ||
                 (crafting_inv.has_amount("welder_crude", 1) &&
                  crafting_inv.has_charges("welder_crude", charges_crude)) ||
                 (crafting_inv.has_amount("toolset", 1) &&
                  crafting_inv.has_charges("toolset", charges / 20));
    has_duct_tape = (crafting_inv.has_charges("duct_tape", DUCT_TAPE_USED));
    has_jack = crafting_inv.has_amount("jack", 1);
    has_siphon = crafting_inv.has_amount("hose", 1);

    has_wheel = crafting_inv.has_amount( "wheel", 1 ) ||
                crafting_inv.has_amount( "wheel_wide", 1 ) ||
                crafting_inv.has_amount( "wheel_bicycle", 1 ) ||
                crafting_inv.has_amount( "wheel_motorbike", 1 ) ||
                crafting_inv.has_amount( "wheel_small", 1 );

    const int iOffsetX = 1 + ((TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    const int iOffsetY = 1 + ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0);
    int mode_x  = iOffsetX;
    int mode_y  = iOffsetY;
    int msg_x   = iOffsetX;
    int msg_y   = iOffsetY;
    int disp_x  = iOffsetX;
    int disp_y  = iOffsetY;
    int parts_x = iOffsetX;
    int parts_y = iOffsetY;
    int stats_x = iOffsetX;
    int stats_y = iOffsetY;
    int list_x  = iOffsetX;
    int list_y  = iOffsetY;
    int name_x  = iOffsetX;
    int name_y  = iOffsetY;

    WINDOW *w_border = newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, -1 + iOffsetY, -1 + iOffsetX );
    //draw_border(...)
    wborder(w_border, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                      LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);

    w_grid  = newwin(FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2, iOffsetY, iOffsetX);

    // Define type of menu:
    std::string menu = OPTIONS["VEH_MENU_STYLE"].getValue();
    int dir = veh->face.dir();
    vertical_menu = (menu == "vertical") ||
                    (menu == "hybrid" && (((dir >= 45) && (dir <= 135)) || ((dir >= 225) && (dir <= 315))));

    if (vertical_menu) {
        //         Vertical menu:
        // +----------------------------+
        // |           w_mode           |
        // |           w_msg            |
        // +--------+---------+---------+
        // | w_disp | w_parts |  w_list |
        // +--------+---------+---------+
        // |          w_name            |
        // |          w_stats           |
        // +----------------------------+
        mode_h  = 1;
        mode_w  = FULL_SCREEN_WIDTH - 2;
        msg_h   = 3;
        msg_w   = FULL_SCREEN_WIDTH - 2;
        disp_h  = 11;
        disp_w  = 15;
        parts_h = disp_h;
        parts_w = 32;
        list_h  = disp_h;
        list_w  = FULL_SCREEN_WIDTH - 1 - disp_w - 1 - parts_w - 1 - 1;
        name_h  = 1;
        name_w  = FULL_SCREEN_WIDTH - 2;
        stats_h = FULL_SCREEN_HEIGHT - 1 - mode_h - msg_h - 1 - disp_h - 1 - name_h - 1;
        stats_w = FULL_SCREEN_WIDTH - 2;

        msg_y   += mode_h;
        disp_y  += mode_h + msg_h + 1;
        parts_y += mode_h + msg_h + 1;
        parts_x += disp_w + 1;
        list_y  += mode_h + msg_h + 1;
        list_x  += disp_w + 1 + parts_w + 1;
        stats_y += mode_h + msg_h + 1 + disp_h + 1 + name_h;
        name_y  += mode_h + msg_h + 1 + disp_h + 1;

        // match grid lines
        mvwputch(w_border, mode_h + msg_h + 1, 0, c_dkgray, LINE_XXXO); // |-
        mvwputch(w_border, mode_h + msg_h + 1, FULL_SCREEN_WIDTH - 1, c_dkgray, LINE_XOXX); // -|
        mvwputch(w_border, mode_h + msg_h + 1 + disp_h + 1, 0, c_dkgray, LINE_XXXO); // |-
        mvwputch(w_border, mode_h + msg_h + 1 + disp_h + 1, FULL_SCREEN_WIDTH - 1, c_dkgray, LINE_XOXX); // -|
    } else {
        //        Horizontal menu:
        // +----------------------------+
        // |           w_name           |
        // +------------------+---------+
        // |      w_disp      |         |
        // +---------+--------+ w_stats |
        // | w_parts | w_list |         |
        // +---------+--------+---------+
        // |           w_mode           |
        // |           w_msg            |
        // +----------------------------+
        name_h  = 1;
        name_w  = FULL_SCREEN_WIDTH - 2;
        parts_h = 9;
        parts_w = 26;
        list_h  = parts_h;
        list_w  = parts_w - 2;
        disp_h  = 6;
        disp_w  = parts_w + 1 + list_w;
        stats_h = disp_h + 1 + parts_h;
        stats_w = FULL_SCREEN_WIDTH - 2 - disp_w - 1;
        mode_h  = 1;
        mode_w  = FULL_SCREEN_WIDTH - 2;
        msg_h   = FULL_SCREEN_HEIGHT - 2 - name_h - 1 - stats_h - 1 - mode_h - 1;
        msg_w   = FULL_SCREEN_WIDTH - 2;

        disp_y  += name_h + 1;
        parts_y += disp_y + disp_h;
        list_y   = parts_y;
        list_x   = parts_w + 1 + 1;
        stats_y  = disp_y;
        stats_x  = disp_w + 1 + 1;
        mode_y  += name_h + 1 + stats_h + 1;
        msg_y   += mode_y + mode_h;

        // match grid lines
        mvwputch(w_border, name_h + 1, 0, c_dkgray, LINE_XXXO); // |-
        mvwputch(w_border, name_h + 1 + disp_h + 1, 0, c_dkgray, LINE_XXXO); // |-
        mvwputch(w_border, name_h + 1 + stats_h + 1, 0, c_dkgray, LINE_XXXO); // |-
        mvwputch(w_border, name_h + 1, FULL_SCREEN_WIDTH - 1, c_dkgray, LINE_XOXX); // -|
        mvwputch(w_border, name_h + 1 + stats_h + 1, FULL_SCREEN_WIDTH - 1, c_dkgray, LINE_XOXX); // -|
    }
    wrefresh(w_border);
    page_size = list_h;

    //               h        w        y        x
    w_mode  = newwin(mode_h,  mode_w,  mode_y,  mode_x );
    w_msg   = newwin(msg_h,   msg_w,   msg_y,   msg_x  );
    w_disp  = newwin(disp_h,  disp_w,  disp_y,  disp_x );
    w_parts = newwin(parts_h, parts_w, parts_y, parts_x);
    w_list  = newwin(list_h,  list_w,  list_y,  list_x );
    w_stats = newwin(stats_h, stats_w, stats_y, stats_x);
    w_name  = newwin(name_h,  name_w,  name_y,  name_x );

    display_grid();
    display_name();
    display_stats();
    move_cursor(0, 0); // display w_disp & w_parts

    bool finish = false;
    while (!finish) {
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction(dx, dy, ch);
        if (ch == KEY_ESCAPE || ch == 'q' ) {
            finish = true;
        } else {
            if (dx != -2 && (dx || dy)) {
                move_cursor(dx, dy);
            } else {
                task_reason reason = cant_do(ch);
                display_mode(ch);
                switch (ch) {
                case 'i':
                    do_install(reason);
                    break;
                case 'r':
                    do_repair(reason);
                    break;
                case 'f':
                    do_refill(reason);
                    break;
                case 'o':
                    do_remove(reason);
                    break;
                case 'e':
                    do_rename(reason);
                    break;
                case 's':
                    do_siphon(reason);
                    break;
                case 'c':
                    do_tirechange(reason);
                    break;
                case 'd':
                    do_drain(reason);
                    break;
                }
                if (sel_cmd != ' ') {
                    finish = true;
                }
                display_mode (' ');
            }
        }
    }
    werase(w_grid);
    werase(w_mode);
    werase(w_msg);
    werase(w_disp);
    werase(w_parts);
    werase(w_stats);
    werase(w_list);
    werase(w_name);
    delwin(w_grid);
    delwin(w_mode);
    delwin(w_msg);
    delwin(w_disp);
    delwin(w_parts);
    delwin(w_stats);
    delwin(w_list);
    delwin(w_name);
    erase();
}

/**
 * Checks if the player is able to perform some command, and returns a nonzero
 * error code if they are unable to perform it. The return from this function
 * should be passed into the various do_whatever functions further down.
 * @param mode The command the player is trying to perform (ie 'r' for repair).
 * @return 0 if the player has everything they need,
 *         1 if the command can't target that square,
 *         2 if the player lacks tools,
 *         3 if something else obstructs the action,
 *         4 if the player's skill isn't high enough.
 */
task_reason veh_interact::cant_do (char mode)
{
    bool valid_target = false;
    bool has_tools = false;
    bool part_free = true;
    bool has_skill = true;
    bool can_remove_wheel = has_wrench && has_jack && wheel;

    switch (mode) {
    case 'i': // install mode
        valid_target = can_mount.size() > 0 && 0 == veh->tags.count("convertible");
        has_tools = has_wrench && (has_welder || has_duct_tape);
        break;
    case 'r': // repair mode
        valid_target = need_repair.size() > 0 && cpart >= 0;
        has_tools = has_welder || has_duct_tape;
        break;
    case 'f': // refill mode
        valid_target = (ptank != NULL && ptank->hp > 0);
        has_tools = has_fuel;
        break;
    case 'o': // remove mode
        valid_target = cpart >= 0 && 0 == veh->tags.count("convertible");
        has_tools = (has_wrench && has_hacksaw) || can_remove_wheel;
        part_free = parts_here.size() > 1 || (cpart >= 0 && veh->can_unmount(cpart));
        has_skill = g->u.skillLevel("mechanics") >= 2 || can_remove_wheel;
        break;
    case 's': // siphon mode
        valid_target = veh->fuel_left("gasoline") > 0;
        has_tools = has_siphon;
        break;
    case 'c': // change tire
        valid_target = wheel != NULL;
        has_tools = has_wrench && has_jack && has_wheel;
        break;
    case 'd': // drain tank
        valid_target = veh->fuel_left("water") > 0;
        has_tools = has_siphon;
        break;
    default:
        return UNKNOWN_TASK;
    }

    if( !valid_target ) {
        return INVALID_TARGET;
    }
    if( !has_tools ) {
        return LACK_TOOLS;
    }
    if( !part_free ) {
        return NOT_FREE;
    }
    if( !has_skill ) {
        return LACK_SKILL;
    }
    return CAN_DO;
}

/**
 * Handles installing a new part.
 * @param reason INVALID_TARGET if the square can't have anything installed,
 *               LACK_TOOLS if the player is lacking tools.
 */
void veh_interact::do_install(task_reason reason)
{
    werase (w_msg);
    int msg_width = getmaxx(w_msg);
    if (g->u.morale_level() < MIN_MORALE_CRAFT) {
        // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, _("Your morale is too low to construct..."));
        wrefresh (w_msg);
        return;
    }
    switch (reason) {
    case INVALID_TARGET:
        mvwprintz(w_msg, 0, 1, c_ltred, _("Cannot install any part here."));
        wrefresh (w_msg);
        return;
    case LACK_TOOLS:
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("You need a <color_%1$s>wrench</color> and either a <color_%2$s>powered welder</color> or <color_%3$s>duct tape</color> to install parts."),
                       has_wrench ? "ltgreen" : "red",
                       has_welder ? "ltgreen" : "red",
                       has_duct_tape ? "ltgreen" : "red");
        wrefresh (w_msg);
        return;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose new part to install here:"));
    wrefresh (w_mode);
    int pos = 0;
    int engines = 0;
    int dif_eng = 0;
    for (int p = 0; p < veh->parts.size(); p++) {
        if (veh->part_flag (p, "ENGINE")) {
            engines++;
            dif_eng = dif_eng / 2 + 12;
        }
    }
    while (true) {
        sel_vpart_info = &(can_mount[pos]);
        display_list (pos, can_mount);
        itype_id itm = sel_vpart_info->item;
        bool has_comps = crafting_inv.has_amount(itm, 1);
        bool has_skill = g->u.skillLevel("mechanics") >= sel_vpart_info->difficulty;
        bool has_tools = (has_welder || has_duct_tape) && has_wrench;
        bool eng = sel_vpart_info->has_flag("ENGINE");
        bool has_skill2 = !eng || (g->u.skillLevel("mechanics") >= dif_eng);
        std::string engine_string = "";
        if (engines && eng) { // already has engine
            engine_string = string_format(
                                _("  You also need level <color_%1$s>%2$d</color> skill in mechanics to install additional engines."),
                                has_skill2 ? "ltgreen" : "red",
                                dif_eng);
        }
        werase (w_msg);
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("Needs <color_%1$s>%2$s</color>, a <color_%3$s>wrench</color>, either a <color_%4$s>powered welder</color> or <color_%5$s>duct tape</color>, and level <color_%6$s>%7$d</color> skill in mechanics.%8$s"),
                       has_comps ? "ltgreen" : "red",
                       itypes[itm]->name.c_str(),
                       has_wrench ? "ltgreen" : "red",
                       has_welder ? "ltgreen" : "red",
                       has_duct_tape ? "ltgreen" : "red",
                       has_skill ? "ltgreen" : "red",
                       sel_vpart_info->difficulty,
                       engine_string.c_str());
        wrefresh (w_msg);
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if ((ch == '\n' || ch == ' ') && has_comps && has_tools && has_skill && has_skill2) {
            sel_cmd = 'i';
            return;
        } else {
            if (ch == KEY_ESCAPE || ch == 'q' ) {
                werase (w_list);
                wrefresh (w_list);
                werase (w_msg);
                wrefresh(w_msg);
                break;
            }
        }
        if (dy == -1 || dy == 1) {
            pos += dy;
            if (pos < 0) {
                pos = can_mount.size() - 1;
            } else if (pos >= can_mount.size()) {
                pos = 0;
            }
        }
    }
}

/**
 * Handles repairing a vehicle part.
 * @param reason INVALID_TARGET if there's no damaged parts in the selected square,
 *               LACK_TOOLS if the player is lacking tools.
 */
void veh_interact::do_repair(task_reason reason)
{
    werase (w_msg);
    int msg_width = getmaxx(w_msg);
    if (g->u.morale_level() < MIN_MORALE_CRAFT) {
        // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, _("Your morale is too low to construct..."));
        wrefresh (w_msg);
        return;
    }
    switch (reason) {
    case INVALID_TARGET:
        if(mostDamagedPart != -1) {
            int p = mostDamagedPart; // for convenience

            int xOffset = veh->parts[p].mount_dy + ddy;
            int yOffset = -(veh->parts[p].mount_dx + ddx);

            move_cursor(xOffset, yOffset);
        } else {
            mvwprintz(w_msg, 0, 1, c_ltred, _("There are no damaged parts on this vehicle."));
            wrefresh (w_msg);
        }
        return;
    case LACK_TOOLS:
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("You need a <color_%1$s>powered welder</color> or <color_%2$s>duct tape</color> to repair."),
                       has_welder ? "ltgreen" : "red",
                       has_duct_tape ? "ltgreen" : "red");
        wrefresh (w_msg);
        return;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose a part here to repair:"));
    wrefresh (w_mode);
    int pos = 0;
    while (true) {
        sel_vehicle_part = &veh->parts[parts_here[need_repair[pos]]];
        werase (w_parts);
        veh->print_part_desc (w_parts, 0, parts_w, cpart, need_repair[pos]);
        wrefresh (w_parts);
        werase (w_msg);
        bool has_comps = true;
        int dif = vehicle_part_types[sel_vehicle_part->id].difficulty + ((sel_vehicle_part->hp <= 0) ? 0 : 2);
        bool has_skill = g->u.skillLevel("mechanics") >= dif;
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("You need level <color_%1$s>%2$d</color> skill in mechanics."),
                       has_skill ? "ltgreen" : "red",
                       dif);
        if (sel_vehicle_part->hp <= 0) {
            itype_id itm = vehicle_part_types[sel_vehicle_part->id].item;
            has_comps = crafting_inv.has_amount(itm, 1);
            fold_and_print(w_msg, 1, 1, msg_width - 2, c_ltgray,
                           _("You also need a <color_%1$s>wrench</color> and <color_%2$s>%3$s</color> to replace broken one."),
                           has_wrench ? "ltgreen" : "red",
                           has_comps ? "ltgreen" : "red",
                           itypes[itm]->name.c_str());
        }
        wrefresh (w_msg);
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if ((ch == '\n' || ch == ' ') &&
            has_comps &&
            (sel_vehicle_part->hp > 0 || has_wrench) && has_skill) {
            sel_cmd = 'r';
            return;
        } else if (ch == KEY_ESCAPE || ch == 'q' ) {
            werase (w_parts);
            veh->print_part_desc (w_parts, 0, parts_w, cpart, -1);
            wrefresh (w_parts);
            werase (w_msg);
            break;
        }
        if (dy == -1 || dy == 1) {
            pos += dy;
            if(pos >= need_repair.size()) {
                pos = 0;
            } else if(pos < 0) {
                pos = need_repair.size() - 1;
            }
        }
    }
}

/**
 * Handles refilling a vehicle's fuel tank.
 * @param reason INVALID_TARGET if there's no fuel tank in the spot,
 *               LACK_TOOLS if the player has nothing to fill the tank with.
 */
void veh_interact::do_refill(task_reason reason)
{
    werase (w_msg);
    int msg_width = getmaxx(w_msg);
    switch (reason) {
    case INVALID_TARGET:
        mvwprintz(w_msg, 0, 1, c_ltred, _("There's no fuel tank here."));
        wrefresh (w_msg);
        return;
    case LACK_TOOLS:
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("You need <color_red>%s</color>."),
                       ammo_name(vehicle_part_types[ptank->id].fuel_type).c_str());
        wrefresh (w_msg);
        return;
    }
    sel_cmd = 'f';
    sel_vehicle_part = ptank;
}

/**
 * Handles removing a part from the vehicle.
 * @param reason INVALID_TARGET if there are no parts to remove,
 *               LACK_TOOLS if the player is lacking tools,
 *               NOT_FREE if there's something attached that needs to be removed first,
 *               LACK_SKILL if the player's mechanics skill isn't high enough.
 */
void veh_interact::do_remove(task_reason reason)
{
    werase (w_msg);
    int msg_width = getmaxx(w_msg);
    if (g->u.morale_level() < MIN_MORALE_CRAFT) {
        // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, _("Your morale is too low to construct..."));
        wrefresh (w_msg);
        return;
    }
    bool can_hacksaw = has_wrench && has_hacksaw &&
                       g->u.skillLevel("mechanics") >= 2;
    switch (reason) {
    case INVALID_TARGET:
        mvwprintz(w_msg, 0, 1, c_ltred, _("No parts here."));
        wrefresh (w_msg);
        return;
    case LACK_TOOLS:
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("You need a <color_%1$s>wrench</color> and a <color_%2$s>hacksaw</color> to remove parts."),
                       has_wrench ? "ltgreen" : "red",
                       has_hacksaw ? "ltgreen" : "red");
        if(wheel) {
            fold_and_print(w_msg, 1, 1, msg_width - 2, c_ltgray,
                           _("To remove a wheel you need a <color_%1$s>wrench</color> and a <color_%2$s>jack</color>."),
                           has_wrench ? "ltgreen" : "red",
                           has_jack ? "ltgreen" : "red");
        }
        wrefresh (w_msg);
        return;
    case NOT_FREE:
        mvwprintz(w_msg, 0, 1, c_ltred,
                  _("You cannot remove that part while something is attached to it."));
        wrefresh (w_msg);
        return;
    case LACK_SKILL:
        mvwprintz(w_msg, 0, 1, c_ltred, _("You need level 2 mechanics skill to remove parts."));
        wrefresh (w_msg);
        return;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose a part here to remove:"));
    wrefresh (w_mode);
    int first = 0;
    int pos = first;
    while (true) {
        sel_vehicle_part = &veh->parts[parts_here[pos]];
        sel_vpart_info = &(vehicle_part_types[sel_vehicle_part->id]);
        bool is_wheel = sel_vpart_info->has_flag("WHEEL");
        werase (w_parts);
        veh->print_part_desc (w_parts, 0, parts_w, cpart, pos);
        wrefresh (w_parts);
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if (ch == '\n' || ch == ' ') {
            if (veh->can_unmount(parts_here[pos])) {
                if (can_hacksaw || is_wheel) {
                    sel_cmd = 'o';
                    return;
                } else {
                    fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                                   _("You need a <color_%1$s>wrench</color> and a <color_%2$s>hacksaw</color> to remove parts."),
                                   has_wrench ? "ltgreen" : "red",
                                   has_hacksaw ? "ltgreen" : "red");
                    wrefresh (w_msg);
                    return;
                }
            } else {
                mvwprintz(w_msg, 0, 1, c_ltred,
                          _("You cannot remove that part while something is attached to it."));
                wrefresh (w_msg);
                return;
            }
        } else if (ch == KEY_ESCAPE || ch == 'q' ) {
            werase (w_parts);
            veh->print_part_desc (w_parts, 0, parts_w, cpart, -1);
            wrefresh (w_parts);
            werase (w_msg);
            break;
        }
        if (dy == -1 || dy == 1) {
            pos += dy;
            if (pos < first) {
                pos = parts_here.size() - 1;
            } else if (pos >= parts_here.size()) {
                pos = first;
            }
        }
    }
}

/**
 * Handles siphoning gas.
 * @param reason INVALID_TARGET if the vehicle has no gas,
 *               NO_TOOLS if the player has no hose.
 */
void veh_interact::do_siphon(task_reason reason)
{
    werase (w_msg);
    int msg_width = getmaxx(w_msg);
    switch (reason) {
    case INVALID_TARGET:
        mvwprintz(w_msg, 0, 1, c_ltred, _("The vehicle has no gasoline to siphon."));
        wrefresh (w_msg);
        return;
    case LACK_TOOLS:
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("You need a <color_red>hose</color> to siphon fuel."));
        wrefresh (w_msg);
        return;
    }
    sel_cmd = 's';
}

/**
 * Handles changing a tire.
 * @param reason INVALID_TARGET if there's no wheel in the selected square,
 *               LACK_TOOLS if the player is missing a tool.
 */
void veh_interact::do_tirechange(task_reason reason)
{
    werase( w_msg );
    int msg_width = getmaxx(w_msg);
    switch( reason ) {
    case INVALID_TARGET:
        mvwprintz(w_msg, 0, 1, c_ltred, _("There is no wheel to change here."));
        wrefresh (w_msg);
        return;
    case LACK_TOOLS:
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("To change a wheel you need a <color_%1$s>wrench</color> and a <color_%2$s>jack</color>."),
                       has_wrench ? "ltgreen" : "red",
                       has_jack ? "ltgreen" : "red");
        wrefresh (w_msg);
        return;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose wheel to use as replacement:"));
    wrefresh (w_mode);
    int pos = 0;
    while (true) {
        sel_vpart_info = &(wheel_types[pos]);
        bool is_wheel = sel_vpart_info->has_flag("WHEEL");
        display_list (pos, wheel_types);
        itype_id itm = sel_vpart_info->item;
        bool has_comps = crafting_inv.has_amount(itm, 1);
        bool has_tools = has_jack && has_wrench;
        werase (w_msg);
        wrefresh (w_msg);
        char ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if ((ch == '\n' || ch == ' ') && has_comps && has_tools && is_wheel) {
            sel_cmd = 'c';
            return;
        } else {
            if (ch == KEY_ESCAPE || ch == 'q' ) {
                werase (w_list);
                wrefresh (w_list);
                werase (w_msg);
                break;
            }
        }
        if (dy == -1 || dy == 1) {
            pos += dy;
            if (pos < 0) {
                pos = wheel_types.size() - 1;
            } else if (pos >= wheel_types.size()) {
                pos = 0;
            }
        }
    }
}

/**
 * Handles draining water from a vehicle.
 * @param reason INVALID_TARGET if the vehicle has no water,
 *               LACK_TOOLS if the player has no hose.
 */
void veh_interact::do_drain(task_reason reason)
{
    werase (w_msg);
    int msg_width = getmaxx(w_msg);
    switch (reason) {
    case INVALID_TARGET:
        mvwprintz(w_msg, 0, 1, c_ltred, _("The vehicle has no water to siphon.") );
        wrefresh (w_msg);
        return;
    case LACK_TOOLS:
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("You need a <color_red>hose</color> to siphon water.") );
        wrefresh (w_msg);
        return;
    }
    sel_cmd = 'd';
}

/**
 * Handles renaming a vehicle.
 * @param reason Unused.
 */
void veh_interact::do_rename(task_reason reason)
{
    std::string name = string_input_popup(_("Enter new vehicle name:"), 20);
    if(name.length() > 0) {
        (veh->name = name);
        if (veh->tracking_on) {
            g->cur_om->vehicles[veh->om_id].name = name;
        }
    }
    display_name();
    display_stats();
    display_grid();
    // refresh w_disp & w_part windows:
    move_cursor(0, 0);
}

/**
 * Returns the first part on the vehicle at the given position.
 * @param dx The x-coordinate, relative to the viewport's 0-point (?)
 * @param dy The y-coordinate, relative to the viewport's 0-point (?)
 * @return The first vehicle part at the specified coordinates.
 */
int veh_interact::part_at (int dx, int dy)
{
    int vdx = -ddx - dy;
    int vdy = dx - ddy;
    return veh->part_displayed_at(vdx, vdy);
}

/**
 * Moves the cursor on the vehicle editing window.
 * @param dx How far to move the cursor on the x-axis.
 * @param dy How far to move the cursor on the y-axis.
 */
void veh_interact::move_cursor (int dx, int dy)
{
    mvwputch (w_disp, (disp_h / 2), (disp_w / 2), cpart >= 0 ? veh->part_color (cpart) : c_black,
              special_symbol(cpart >= 0 ? veh->part_sym (cpart) : ' '));

    if (vertical_menu) {
        ddx += dy;
        ddy -= dx;
    } else {
        ddx -= dx;
        ddy -= dy;
    }
    display_veh();
    cpart = part_at (0, 0);
    int vdx = -ddx;
    int vdy = -ddy;
    int vx, vy;
    veh->coord_translate (vdx, vdy, vx, vy);
    int vehx = veh->global_x() + vx;
    int vehy = veh->global_y() + vy;
    bool obstruct = g->m.move_cost_ter_furn (vehx, vehy) == 0;
    vehicle *oveh = g->m.veh_at (vehx, vehy);
    if (oveh && oveh != veh) {
        obstruct = true;
    }
    nc_color col = cpart >= 0 ? veh->part_color (cpart) : c_black;
    mvwputch (w_disp, (disp_h / 2), (disp_w / 2), obstruct ? red_background(col) : hilite(col),
              special_symbol(cpart >= 0 ? veh->part_sym (cpart) : ' '));
    wrefresh (w_disp);
    werase (w_parts);
    veh->print_part_desc (w_parts, 0, parts_w, cpart, -1);
    wrefresh (w_parts);

    can_mount.clear();
    if (!obstruct) {
        for (std::map<std::string, vpart_info>::iterator
             part_type_iterator = vehicle_part_types.begin();
             part_type_iterator != vehicle_part_types.end();
             ++part_type_iterator) {
            if (veh->can_mount (vdx, vdy, part_type_iterator->first)) {
                can_mount.push_back (part_type_iterator->second);
            }
        }
    }

    //Only build the wheel list once
    if (wheel_types.empty()) {
        for (std::map<std::string, vpart_info>::iterator
             part_type_iterator = vehicle_part_types.begin();
             part_type_iterator != vehicle_part_types.end();
             ++part_type_iterator) {
            if (part_type_iterator->second.has_flag("WHEEL")) {
                wheel_types.push_back (part_type_iterator->second);
            }
        }
    }

    need_repair.clear();
    parts_here.clear();
    ptank = NULL;
    wheel = NULL;
    if (cpart >= 0) {
        parts_here = veh->parts_at_relative(veh->parts[cpart].mount_dx, veh->parts[cpart].mount_dy);
        for (int i = 0; i < parts_here.size(); i++) {
            int p = parts_here[i];
            if (veh->parts[p].hp < veh->part_info(p).durability) {
                need_repair.push_back (i);
            }
            if (veh->part_flag(p, "FUEL_TANK") && veh->parts[p].amount < veh->part_info(p).size) {
                ptank = &veh->parts[p];
            }
            if (veh->part_flag(p, "WHEEL")) {
                wheel = &veh->parts[p];
            }
        }
    }
    has_fuel = ptank != NULL ? g->refill_vehicle_part(*veh, ptank, true) : false;
    werase (w_msg);
    wrefresh (w_msg);
    display_mode (' ');
}

void veh_interact::display_grid()
{
    if (vertical_menu) {
        // Two lines dividing the three middle sections.
        for (int i = 1 + mode_h + msg_h; i < (1 + mode_h + msg_h + disp_h); i++) {
            mvwputch(w_grid, i, disp_w, c_dkgray, LINE_XOXO); // |
            mvwputch(w_grid, i, disp_w + 1 + parts_w, c_dkgray, LINE_XOXO); // |
        }
        // Two lines dividing the vertical menu sections.
        for (int i = 0; i < w_grid->width; i++) {
            mvwputch( w_grid, mode_h + msg_h, i, c_dkgray, LINE_OXOX ); // -
            mvwputch( w_grid, mode_h + msg_h + 1 + disp_h, i, c_dkgray, LINE_OXOX ); // -
        }
        // Fix up the line intersections.
        mvwputch(w_grid, mode_h + msg_h,              disp_w, c_dkgray, LINE_OXXX);
        mvwputch(w_grid, mode_h + msg_h + 1 + disp_h, disp_w, c_dkgray, LINE_XXOX); // _|_
        mvwputch(w_grid, mode_h + msg_h,              disp_w + 1 + parts_w, c_dkgray, LINE_OXXX);
        mvwputch(w_grid, mode_h + msg_h + 1 + disp_h, disp_w + 1 + parts_w, c_dkgray, LINE_XXOX); // _|_
    } else {
        // Vertical lines
        for (int i = name_h + 1; i < (name_h + 1 + disp_h + 1 + parts_h); i++) {
            mvwputch(w_grid, i, disp_w, c_dkgray, LINE_XOXO); // |
        }
        for (int i = (name_h + 1 + disp_h + 1); i < (name_h + 1 + stats_h); i++) {
            mvwputch(w_grid, i, parts_w, c_dkgray, LINE_XOXO); // |
        }     
        
        // Two horizontal lines: one after name window, and another after parts window
        for (int i = 0; i < FULL_SCREEN_WIDTH; i++) {
            mvwputch(w_grid, name_h, i, c_dkgray, LINE_OXOX);
            mvwputch(w_grid, name_h + 1 + stats_h, i, c_dkgray, LINE_OXOX);
        }
        // Horizontal line between vehicle/parts windows 
        for (int i = 0; i < disp_w; i++) {
            mvwputch(w_grid, name_h + 1 + disp_h, i, c_dkgray, LINE_OXOX); 
        }
        // Fix up the line intersections.
        mvwputch(w_grid, name_h, disp_w, c_dkgray, LINE_OXXX);
        mvwputch(w_grid, name_h + 1 + disp_h, parts_w, c_dkgray, LINE_OXXX);
        mvwputch(w_grid, name_h + 1 + disp_h + parts_h + 1, parts_w, c_dkgray, LINE_XXOX);
        mvwputch(w_grid, name_h + 1 + disp_h, disp_w, c_dkgray, LINE_XOXX); // -|
        mvwputch(w_grid, name_h + 1 + stats_h, disp_w, c_dkgray, LINE_XXOX );
    }
    wrefresh(w_grid);
}

/**
 * Draws the viewport with the vehicle.
 */
void veh_interact::display_veh ()
{
    werase(w_disp);
    //Iterate over structural parts so we only hit each square once
    std::vector<int> structural_parts = veh->all_parts_at_location("structure");
    int x, y;
    for (int i = 0; i < structural_parts.size(); i++) {
        const int p = structural_parts[i];
        long sym = veh->part_sym (p);
        nc_color col = veh->part_color (p);
        if (vertical_menu) {
            x =   veh->parts[p].mount_dy + ddy;
            y = -(veh->parts[p].mount_dx + ddx);
        } else {
            x = veh->parts[p].mount_dx + ddx;
            y = veh->parts[p].mount_dy + ddy;
        }
        if (x == 0 && y == 0) {
            col = hilite(col);
            cpart = p;
        }
        mvwputch (w_disp, (disp_h / 2) + y, (disp_w / 2) + x, col, special_symbol(sym));
    }
    if (!vertical_menu) {
        size_t len = utf8_width(_("FWD ->"));
        mvwprintz(w_disp, 0, disp_w - len, c_dkgray,  _("FWD ->"));
    /* } else {
        // not enough space for this arrow
        size_t len = utf8_width(_("FWD"));
        mvwprintz(w_disp, 0, disp_w - len, c_dkgray,  _("FWD"));
        mvwprintz(w_disp, 1, disp_w - 2,   c_dkgray,   _("^ "));
        mvwprintz(w_disp, 2, disp_w - 2,   c_dkgray,   _("| "));
    */
    }
    wrefresh (w_disp);
}

/**
 * Displays the vehicle's stats at the bottom of the window.
 */
void veh_interact::display_stats()
{
    int safe_vel_x;
    int safe_vel_y;
    int safe_vel_w;
    int top_vel_x;
    int top_vel_y;
    int top_vel_w;
    int acc_x;
    int acc_y;
    int acc_w;
    int mass_x;
    int mass_y;
    int mass_w;
    int k_dyn_x;
    int k_dyn_y;
    int k_dyn_w;
    int k_mass_x;
    int k_mass_y;
    int k_mass_w;
    int wheels_x;
    int wheels_y;
    int wheels_w;
    int fuel_use_x;
    int fuel_use_y;
    int fuel_ind_x;
    int fuel_ind_y;
    int status_x;
    int status_y;
    int status_w;
    int dmg_prt_x;
    int dmg_prt_y;
    int dmg_prt_w;

    if (vertical_menu) {
        // Vertical menu
        const int second_column = 34;
        const int third_column  = 63;
        // Y-coordinates for vertical menu
        safe_vel_y = 0;
        top_vel_y  = safe_vel_y + 1;
        acc_y      = top_vel_y + 1;
        mass_y     = acc_y + 1;
        status_y   = mass_y + 1;

        fuel_use_y = 0;
        k_dyn_y    = fuel_use_y + 1;
        k_mass_y   = k_dyn_y + 1;
        dmg_prt_y  = k_mass_y + 1;
        wheels_y   = dmg_prt_y + 1;

        fuel_ind_y = 0;

        // X-coordinates for vertical menu
        safe_vel_x = 1;
        top_vel_x  = 1;
        acc_x      = 1;
        mass_x     = 1;
        status_x   = 1;

        fuel_use_x = second_column;
        k_dyn_x    = second_column;
        k_mass_x   = second_column;
        wheels_x   = second_column;
        dmg_prt_x  = second_column;

        fuel_ind_x = third_column;

        // Width for vertical menu
        safe_vel_w = second_column;
        top_vel_w  = second_column;
        acc_w      = second_column;
        mass_w     = second_column;
        status_w   = second_column;

        k_dyn_w   = third_column;
        k_mass_w  = third_column;
        wheels_w  = third_column;
        dmg_prt_w = third_column;
    } else {
        // Y-coordinates for horizontal menu
        safe_vel_y = 0;
        top_vel_y  = safe_vel_y + 1;
        acc_y      = top_vel_y + 1;
        mass_y     = acc_y + 1;
        k_mass_y   = mass_y + 1;
        k_dyn_y    = k_mass_y + 1;
        wheels_y   = k_dyn_y + 1;
        status_y   = wheels_y + 1;
        dmg_prt_y  = status_y + 1;

        fuel_use_y = dmg_prt_y + 1;
        fuel_ind_y = fuel_use_y + 1;

        // X-coordinates for horizontal menu
        safe_vel_x = 1;
        top_vel_x  = 1;
        acc_x      = 1;
        mass_x     = 1;
        k_dyn_x    = 1;
        k_mass_x   = 1;
        wheels_x   = 1;
        fuel_use_x = 1;
        fuel_ind_x = 1;
        status_x   = 1;
        dmg_prt_x  = 1;

        // Width for horizontal menu
        safe_vel_w = w_stats->width - 1;
        top_vel_w  = w_stats->width - 1;
        acc_w      = w_stats->width - 1;
        mass_w     = w_stats->width - 1;
        k_dyn_w    = w_stats->width - 1;
        k_mass_w   = w_stats->width - 1;
        wheels_w   = w_stats->width - 1;
        status_w   = w_stats->width - 1;
        dmg_prt_w  = w_stats->width - 1;
    }
    bool conf = veh->valid_wheel_config();
    std::string speed_units = OPTIONS["USE_METRIC_SPEEDS"].getValue();
    float speed_factor = 0.01f;
    if (speed_units == "km/h") {
        speed_factor *= 1.61f;
    }
    std::string weight_units = OPTIONS["USE_METRIC_WEIGHTS"].getValue();
    float weight_factor = 1.0f;
    if (weight_units == "lbs") {
        weight_factor *= 2.2f;
    }
    fold_and_print(w_stats, safe_vel_y, safe_vel_x, safe_vel_w, c_ltgray,
                   _("Safe speed:   <color_ltgreen>%3d</color> %s"),
                   int(veh->safe_velocity(false) * speed_factor), speed_units.c_str());
    fold_and_print(w_stats, top_vel_y, top_vel_x, top_vel_w, c_ltgray,
                   _("Top speed:    <color_ltred>%3d</color> %s"),
                   int(veh->max_velocity(false) * speed_factor), speed_units.c_str());
    fold_and_print(w_stats, acc_y, acc_x, acc_w, c_ltgray,
                   _("Acceleration: <color_ltblue>%3d</color> %s/t"),
                   int(veh->acceleration(false) * speed_factor), speed_units.c_str());
    fold_and_print(w_stats, mass_y, mass_x, mass_w, c_ltgray,
                   _("Mass:       <color_ltblue>%5d</color> %s"),
                   int(veh->total_mass() * weight_factor), weight_units.c_str());
    if (conf) {
        fold_and_print(w_stats, wheels_y, wheels_x, wheels_w, c_ltgray,
                       _("Wheels:    <color_ltgreen>enough</color>"));
    } else {
        fold_and_print(w_stats, wheels_y, wheels_x, wheels_w, c_ltgray,
                       _("Wheels:      <color_ltred>lack</color>"));
    }

    fold_and_print(w_stats, k_dyn_y, k_dyn_x, k_dyn_w, c_ltgray,
                   _("K dynamics:   <color_ltblue>%3d</color>%%"),
                   int(veh->k_dynamics() * 100));
    fold_and_print(w_stats, k_mass_y, k_mass_x, k_mass_w, c_ltgray,
                   _("K mass:       <color_ltblue>%3d</color>%%"),
                   int(veh->k_mass() * 100));
    
    // "Fuel usage (safe): " is renamed to "Fuel usage: ". 
    mvwprintz(w_stats, fuel_use_y, fuel_use_x, c_ltgray,  _("Fuel usage:     "));
    fuel_use_x += utf8_width(_("Fuel usage:     "));
    ammotype fuel_types[3] = { "gasoline", "battery", "plasma" };
    nc_color fuel_colors[3] = { c_ltred, c_yellow, c_ltblue };
    bool first = true;
    for (int i = 0; i < 3; ++i) {
        int fuel_usage = veh->basic_consumption(fuel_types[i]);
        if (fuel_usage > 0) {
            fuel_usage = fuel_usage / 100;
            if (fuel_usage < 1) {
                fuel_usage = 1;
            }
            if (!first) {
                mvwprintz(w_stats, fuel_use_y, fuel_use_x++, c_ltgray, "/");
            }
            mvwprintz(w_stats, fuel_use_y, fuel_use_x++, fuel_colors[i], "%d", fuel_usage);
            if (fuel_usage > 9) {
                fuel_use_x++;
            }
            if (fuel_usage > 99) {
                fuel_use_x++;
            }
            first = false;
        }
        if (first) {
            mvwprintz(w_stats, fuel_use_y, fuel_use_x, c_ltgray, "-"); // no engines
        }
    }
    veh->print_fuel_indicator (w_stats, fuel_ind_y, fuel_ind_x, true, true);

    // Write the overall damage
    mvwprintz(w_stats, status_y, status_x, c_ltgray, _("Status:  "));
    status_x += utf8_width(_("Status: ")) + 1;
    fold_and_print(w_stats, status_y, status_x, status_w, totalDurabilityColor, totalDurabilityText.c_str());

    // Write the most damaged part
    if (mostDamagedPart != -1) {
        std::string partName;
        mvwprintz(w_stats, dmg_prt_y, dmg_prt_x, c_ltgray, _("Most damaged: "));
        dmg_prt_x += utf8_width(_("Most damaged: ")) + 1;
        std::string partID = veh->parts[mostDamagedPart].id;
        vehicle_part part = veh->parts[mostDamagedPart];
        int damagepercent = part.hp / vehicle_part_types[part.id].durability;
        nc_color damagecolor = getDurabilityColor(damagepercent * 100);
        partName = vehicle_part_types[partID].name;
        fold_and_print(w_stats, dmg_prt_y, dmg_prt_x, dmg_prt_w, damagecolor, "%s", partName.c_str());
    }

    wrefresh(w_stats);
}

void veh_interact::display_name()
{
    werase(w_name);
    mvwprintz(w_name, 0, 1, c_ltgray, _("Name: "));
    mvwprintz(w_name, 0, 1 + utf8_width(_("Name: ")), c_ltgreen, veh->name.c_str());
    if (!vertical_menu) {
        display_esc(w_name);
    }
    wrefresh(w_name);
}

/**
 * Prints the list of usable commands, and highlights the hotkeys used to activate them.
 * @param mode What command we are currently using. ' ' for no command.
 */
void veh_interact::display_mode(char mode)
{
    werase (w_mode);

    size_t esc_pos;
    if (vertical_menu) {
        esc_pos = display_esc(w_mode);
    } else {
        esc_pos = w_mode->width;
    }

    if (mode == ' ') {
        std::vector<std::string> actions;
        actions.push_back(_("<i>nstall"));
        actions.push_back(_("<r>epair"));
        actions.push_back(_("re<f>ill"));
        actions.push_back(_("rem<o>ve"));
        actions.push_back(_("<s>iphon"));
        actions.push_back(_("<d>rain water"));
        actions.push_back(_("<c>hange tire"));
        actions.push_back(_("r<e>name"));

        bool enabled[8];
        enabled[0] = !cant_do('i');
        enabled[1] = !cant_do('r');
        enabled[2] = !cant_do('f');
        enabled[3] = !cant_do('o');
        enabled[4] = !cant_do('s');
        enabled[5] = !cant_do('d');
        enabled[6] = !cant_do('c');
        enabled[7] = true;          // 'rename' is always available

        int pos[9];
        pos[0] = 1;
        for (size_t i = 0; i < actions.size(); i++) {
            pos[i+1] = pos[i] + utf8_width(actions[i].c_str()) - 2;
        }
        int spacing = int((esc_pos - 1 - pos[actions.size()]) / actions.size());
        int shift = int((esc_pos - pos[actions.size()] - spacing * (actions.size() - 1)) / 2) - 1;
        for (size_t i = 0; i < actions.size(); i++) {
            shortcut_print(w_mode, 0, pos[i] + spacing * i + shift,
                           enabled[i]? c_ltgray : c_dkgray, enabled[i]? c_ltgreen : c_green,
                           actions[i].c_str());
        }
    }
    wrefresh (w_mode);
}

size_t veh_interact::display_esc(WINDOW *win)
{
    std::string backstr = _("<ESC>-back");
    size_t pos = win->width - utf8_width(backstr.c_str()) + 2;    // right text align
    shortcut_print(win, 0, pos, c_ltgray, c_ltgreen, backstr.c_str());
    return pos;
    wrefresh(win);
}

/**
 * Draws the list of parts that can be mounted in the selected square. Used
 * when installing new parts or changing tires.
 * @param pos The current cursor position in the list.
 * @param list The list to display parts from.
 */
void veh_interact::display_list(int pos, std::vector<vpart_info> list)
{
    werase (w_list);
    int page = pos / page_size;
    for (int i = page * page_size; i < (page + 1) * page_size && i < list.size(); i++) {
        int y = i - page * page_size;
        itype_id itm = list[i].item;
        bool has_comps = crafting_inv.has_amount(itm, 1);
        bool has_skill = g->u.skillLevel("mechanics") >= list[i].difficulty;
        bool is_wheel = list[i].has_flag("WHEEL");
        nc_color col = has_comps && (has_skill || is_wheel) ? c_white : c_dkgray;
        mvwprintz(w_list, y, 3, pos == i ? hilite (col) : col, list[i].name.c_str());
        mvwputch (w_list, y, 1, list[i].color, special_symbol(list[i].sym));
    }
    wrefresh (w_list);
}

void veh_interact::countDurability()
{
    int sum = 0; // sum of part HP
    int max = 0; // sum of part max HP, ie durability
    double mostDamaged = 1; // durability ratio of the most damaged part

    for (int it = 0; it < veh->parts.size(); it++) {
        vehicle_part part = veh->parts[it];
        int part_dur = vehicle_part_types[part.id].durability;

        sum += part.hp;
        max += part_dur;

        if(part.hp < part_dur) {
            double damageRatio = (double) part.hp / part_dur;
            if (!ISNAN(damageRatio) && (damageRatio < mostDamaged)) {
                mostDamaged = damageRatio;
                mostDamagedPart = it;
            }
        }
    }

    double totalDamagePercent = sum / (double)max;
    durabilityPercent = totalDamagePercent * 100;

    totalDurabilityColor = getDurabilityColor(durabilityPercent);
    totalDurabilityText = getDurabilityDescription(durabilityPercent);
}

nc_color veh_interact::getDurabilityColor(const int &dur)
{
    if (dur >= 95) {
        return c_green;
    }
    if (dur >= 66) {
        return c_ltgreen;
    }
    if (dur >= 33) {
        return c_yellow;
    }
    if (dur >= 10) {
        return c_ltred;
    }
    if (dur > 0) {
        return c_red;
    }
    if (dur == 0) {
        return c_dkgray;
    }

    return c_black_yellow;
}

std::string veh_interact::getDurabilityDescription(const int &dur)
{
    if (dur >= 95) {
        return std::string(_("like new"));
    }
    if (dur >= 66) {
        return std::string(_("dented"));
    }
    if (dur >= 33) {
        return std::string(_("battered"));
    }
    if (dur >= 10) {
        return std::string(_("wrecked"));
    }
    if (dur > 0) {
        return std::string(_("totaled"));
    }
    if (dur == 0) {
        return std::string(_("destroyed"));
    }

    return std::string(_("error"));
}


/** Used by consume_vpart_item to track items that could be consumed. */
struct candidate_vpart {
    bool in_inventory;
    int mapx;
    int mapy;
    union {
        signed char invlet;
        int index;
    };
    item vpart_item;
    candidate_vpart(int x, int y, int i, item vpitem):
        in_inventory(false), mapx(x), mapy(y), index(i) {
        vpart_item = vpitem;
    }
    candidate_vpart(char ch, item vpitem):
        in_inventory(true), mapx(-1), mapy(-1), invlet(ch) {
        vpart_item = vpitem;
    }
};

/**
 * Given a vpart id, gives the choice of inventory and nearby items to consume
 * for install/repair/etc. Doesn't use consume_items in crafting.cpp, as it got
 * into weird cases and doesn't consider properties like HP and bigness. The
 * item will be removed by this function.
 * @param vpid The id of the vpart type to look for.
 * @return The item that was consumed.
 */
item consume_vpart_item (game *g, std::string vpid)
{
    std::vector<candidate_vpart> candidates;
    const itype_id itid = vehicle_part_types[vpid].item;
    for (int x = g->u.posx - PICKUP_RANGE; x <= g->u.posx + PICKUP_RANGE; x++) {
        for (int y = g->u.posy - PICKUP_RANGE; y <= g->u.posy + PICKUP_RANGE; y++) {
            for(int i = 0; i < g->m.i_at(x, y).size(); i++) {
                item *ith_item = &(g->m.i_at(x, y)[i]);
                if (ith_item->type->id == itid) {
                    candidates.push_back (candidate_vpart(x, y, i, *ith_item));
                }
            }
        }
    }

    std::vector<item *> cand_from_inv = g->u.inv.all_items_by_type(itid);
    for (int i = 0; i < cand_from_inv.size(); i++) {
        item *ith_item = cand_from_inv[i];
        if (ith_item->type->id  == itid) {
            candidates.push_back (candidate_vpart(ith_item->invlet, *ith_item));
        }
    }
    if (g->u.weapon.type->id == itid) {
        candidates.push_back (candidate_vpart(-1, g->u.weapon));
    }

    // bug?
    if(candidates.size() == 0) {
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
        for(int i = 0; i < candidates.size(); i++) {
            if(candidates[i].in_inventory) {
                if (candidates[i].invlet == -1) {
                    options.push_back(candidates[i].vpart_item.tname() + _(" (wielded)"));
                } else {
                    options.push_back(candidates[i].vpart_item.tname());
                }
            } else {
                //nearby.
                options.push_back(candidates[i].vpart_item.tname() + _(" (nearby)"));
            }
        }
        selection = menu_vec(false, _("Use which gizmo?"), options);
        selection -= 1;
    }
    //remove item from inventory. or map.
    if(candidates[selection].in_inventory) {
        if(candidates[selection].invlet == -1) { //weapon
            g->u.remove_weapon();
        } else { //non-weapon inventory
            g->u.inv.remove_item_by_letter(candidates[selection].invlet);
        }
    } else {
        //map.
        int x = candidates[selection].mapx;
        int y = candidates[selection].mapy;
        int i = candidates[selection].index;
        g->m.i_rem(x, y, i);
    }
    return candidates[selection].vpart_item;
}

/**
 * Called when the activity timer for installing parts, repairing, etc times
 * out and the the action is complete.
 */
void complete_vehicle (game *g)
{
    if (g->u.activity.values.size() < 8) {
        debugmsg ("Invalid activity ACT_VEHICLE values:%d", g->u.activity.values.size());
        return;
    }
    vehicle *veh = g->m.veh_at (g->u.activity.values[0], g->u.activity.values[1]);
    if (!veh) {
        debugmsg ("Activity ACT_VEHICLE: vehicle not found");
        return;
    }
    char cmd = (char) g->u.activity.index;
    int dx = g->u.activity.values[4];
    int dy = g->u.activity.values[5];
    int vehicle_part = g->u.activity.values[6];
    int type = g->u.activity.values[7];
    std::string part_id = g->u.activity.str_values[0];
    std::vector<component> tools;
    int welder_charges = static_cast<it_tool *>(itypes["welder"])->charges_per_use;
    int welder_crude_charges = static_cast<it_tool *>(itypes["welder_crude"])->charges_per_use;
    int partnum;
    item used_item;
    bool broken;
    int replaced_wheel;
    std::vector<int> parts;
    int dd = 2;
    switch (cmd) {
    case 'i':
        partnum = veh->install_part (dx, dy, part_id);
        if(partnum < 0) {
            debugmsg ("complete_vehicle install part fails dx=%d dy=%d id=%d", dx, dy, part_id.c_str());
        }
        used_item = consume_vpart_item (g, part_id);
        veh->get_part_properties_from_item(g, partnum, used_item); //transfer damage, etc.
        tools.push_back(component("welder", welder_charges));
        tools.push_back(component("welder_crude", welder_crude_charges));
        tools.push_back(component("duct_tape", DUCT_TAPE_USED));
        tools.push_back(component("toolset", welder_charges / 20));
        g->consume_tools(&g->u, tools, true);

        if ( vehicle_part_types[part_id].has_flag("CONE_LIGHT") ) {
            // Need map-relative coordinates to compare to output of look_around.
            int gx, gy;
            // Need to call coord_translate() directly since it's a new part.
            veh->coord_translate(dx, dy, gx, gy);
            // Stash offset and set it to the location of the part so look_around will start there.
            int px = g->u.view_offset_x;
            int py = g->u.view_offset_y;
            g->u.view_offset_x = veh->global_x() + gx - g->u.posx;
            g->u.view_offset_y = veh->global_y() + gy - g->u.posy;
            popup(_("Choose a facing direction for the new headlight."));
            point headlight_target = g->look_around();
            // Restore previous view offsets.
            g->u.view_offset_x = px;
            g->u.view_offset_y = py;

            int delta_x = headlight_target.x - (veh->global_x() + gx);
            int delta_y = headlight_target.y - (veh->global_y() + gy);

            const double PI = 3.14159265358979f;
            int dir = int(atan2(static_cast<float>(delta_y), static_cast<float>(delta_x)) * 180.0 / PI);
            dir -= veh->face.dir();
            while(dir < 0) {
                dir += 360;
            }
            while(dir > 360) {
                dir -= 360;
            }

            veh->parts[partnum].direction = dir;
        }

        g->add_msg (_("You install a %s into the %s."),
                    vehicle_part_types[part_id].name.c_str(), veh->name.c_str());
        g->u.practice (g->turn, "mechanics", vehicle_part_types[part_id].difficulty * 5 + 20);
        break;
    case 'r':
        if (veh->parts[vehicle_part].hp <= 0) {
            veh->break_part_into_pieces(vehicle_part, g->u.posx, g->u.posy);
            used_item = consume_vpart_item (g, veh->parts[vehicle_part].id);
            veh->parts[vehicle_part].bigness = used_item.bigness;
            tools.push_back(component("wrench", -1));
            g->consume_tools(&g->u, tools, true);
            tools.clear();
            dd = 0;
            veh->insides_dirty = true;
        }
        tools.push_back(component("welder", welder_charges));
        tools.push_back(component("welder_crude", welder_crude_charges));
        tools.push_back(component("duct_tape", DUCT_TAPE_USED));
        tools.push_back(component("toolset", welder_charges / 20));
        g->consume_tools(&g->u, tools, true);
        veh->parts[vehicle_part].hp = veh->part_info(vehicle_part).durability;
        g->add_msg (_("You repair the %s's %s."),
                    veh->name.c_str(), veh->part_info(vehicle_part).name.c_str());
        g->u.practice (g->turn, "mechanics", (veh->part_info(vehicle_part).difficulty + dd) * 5 + 20);
        break;
    case 'f':
        if (!g->pl_refill_vehicle(*veh, vehicle_part, true)) {
            debugmsg ("complete_vehicle refill broken");
        }
        g->pl_refill_vehicle(*veh, vehicle_part);
        break;
    case 'o':
        // Dump contents of part at player's feet, if any.
        for (int i = 0; i < veh->parts[vehicle_part].items.size(); i++) {
            g->m.add_item_or_charges (g->u.posx, g->u.posy, veh->parts[vehicle_part].items[i]);
        }
        veh->parts[vehicle_part].items.clear();

        broken = veh->parts[vehicle_part].hp <= 0;
        if (!broken) {
            used_item = veh->item_from_part( vehicle_part );
            // Transfer fuel back to tank
            if (used_item.typeId() == "metal_tank") {
                ammotype desired_liquid = veh->part_info(vehicle_part).fuel_type;
                item liquid( itypes[default_ammo(desired_liquid)], g->turn );

                liquid.charges = veh->parts[vehicle_part].amount;
                veh->parts[vehicle_part].amount = 0;

                used_item.put_in(liquid);
            }
            g->m.add_item_or_charges(g->u.posx, g->u.posy, used_item);
            if(type != SEL_JACK) { // Changing tires won't make you a car mechanic
                g->u.practice (g->turn, "mechanics", 2 * 5 + 20);
            }
        } else {
            veh->break_part_into_pieces(vehicle_part, g->u.posx, g->u.posy);
        }
        if (veh->parts.size() < 2) {
            g->add_msg (_("You completely dismantle the %s."), veh->name.c_str());
            g->u.activity.type = ACT_NULL;
            g->m.destroy_vehicle (veh);
        } else {
            if (broken) {
                g->add_msg(_("You remove the broken %s from the %s."),
                           veh->part_info(vehicle_part).name.c_str(),
                           veh->name.c_str());
            } else {
                g->add_msg(_("You remove the %s from the %s."),
                           veh->part_info(vehicle_part).name.c_str(),
                           veh->name.c_str());
            }
            veh->remove_part (vehicle_part);
        }
        break;
    case 's':
        g->u.siphon( g, veh, "gasoline" );
        break;
    case 'c':
        parts = veh->parts_at_relative( dx, dy );
        if( parts.size() ) {
            item removed_wheel;
            replaced_wheel = veh->part_with_feature( parts[0], "WHEEL", false );
            if( replaced_wheel == -1 ) {
                debugmsg( "no wheel to remove when changing wheels." );
                return;
            }
            broken = veh->parts[replaced_wheel].hp <= 0;
            removed_wheel = veh->item_from_part( replaced_wheel );
            veh->remove_part( replaced_wheel );
            g->add_msg( _("You replace one of the %s's tires with a %s."),
                        veh->name.c_str(), vehicle_part_types[part_id].name.c_str() );
            partnum = veh->install_part( dx, dy, part_id );
            if( partnum < 0 ) {
                debugmsg ("complete_vehicle tire change fails dx=%d dy=%d id=%d", dx, dy, part_id.c_str());
            }
            used_item = consume_vpart_item( g, part_id );
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
    }
}

