#include <string>
#include "veh_interact.h"
#include "vehicle.h"
#include "overmapbuffer.h"
#include "game.h"
#include "output.h"
#include "catacharset.h"
#include "crafting.h"
#include "options.h"
#include "debug.h"
#include "messages.h"
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
    : main_context("VEH_INTERACT")
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

    main_context.register_directions();
    main_context.register_action("QUIT");
    main_context.register_action("INSTALL");
    main_context.register_action("REPAIR");
    main_context.register_action("REFILL");
    main_context.register_action("REMOVE");
    main_context.register_action("RENAME");
    main_context.register_action("SIPHON");
    main_context.register_action("TIRE_CHANGE");
    main_context.register_action("DRAIN");
    main_context.register_action("RELABEL");
    main_context.register_action("PREV_TAB");
    main_context.register_action("NEXT_TAB");
    main_context.register_action("CONFIRM");
    main_context.register_action("HELP_KEYBINDINGS");
}

/**
 * Creates a veh_interact window based on the given parameters.
 * @param v The vehicle the player is interacting with.
 */
void veh_interact::exec(vehicle *v)
{
    veh = v;
    countDurability();
    cache_tool_availability();
    allocate_windows();
    do_main_loop();
    deallocate_windows();
}

void veh_interact::allocate_windows()
{
    // main window should also expand to use available display space.
    // expanding to evenly use up half of extra space, for now.
    const int extra_w = ((TERMX - FULL_SCREEN_WIDTH) / 4) * 2;
    const int extra_h = ((TERMY - FULL_SCREEN_HEIGHT) / 4) * 2;
    const int total_w = FULL_SCREEN_WIDTH + extra_w;
    const int total_h = FULL_SCREEN_HEIGHT + extra_h;

    // position of window within main display
    const int x0 = (TERMX - total_w) / 2;
    const int y0 = (TERMY - total_h) / 2;

    // border window
    WINDOW *w_border = newwin(total_h, total_w, y0, x0);
    draw_border(w_border);

    // grid window
    const int grid_w = total_w - 2; // exterior borders take 2
    const int grid_h = total_h - 2; // exterior borders take 2
    w_grid = newwin(grid_h, grid_w, y0 + 1, x0 + 1);

    // Define type of menu:
    std::string menu = OPTIONS["VEH_MENU_STYLE"].getValue();
    int dir = veh->face.dir();
    vertical_menu = (menu == "vertical") ||
                    (menu == "hybrid" && (((dir >= 45) && (dir <= 135)) || ((dir >= 225) && (dir <= 315))));

    int mode_x, mode_y, msg_x, msg_y, disp_x, disp_y, parts_x, parts_y;
    int stats_x, stats_y, list_x, list_y, name_x, name_y, req_x, req_y;

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
        //
        // w_disp/w_parts/w_list expand to take up extra height.
        // w_disp, w_parts and w_list share extra width in a 2:1:1 ratio,
        // but w_parts and w_list start with more than w_disp.

        const int h1 = 4; // 4 lines for msg + mode
        const int h3 = 6; // 6 lines for name + stats

        mode_h = 1;
        mode_w = grid_w;
        msg_h = h1 - mode_h;
        msg_w = mode_w;

        name_h = 1;
        name_w = grid_w;
        stats_h = h3 - name_h;
        stats_w = grid_w;

        list_h = grid_h - h3 - h1 - 2; // interior borders take 2
        list_w = 32 + (extra_w / 4); // uses 1/4 of extra space
        parts_h = list_h;
        parts_w = 32 + (extra_w / 4); // uses 1/4 of extra space

        disp_h = list_h;
        disp_w = grid_w - list_w - parts_w - 2; // interior borders take 2

        mode_x = x0 + 1;
        mode_y = y0 + 1;
        msg_x = x0 + 1;
        msg_y = mode_y + mode_h;
        disp_x = x0 + 1;
        disp_y = y0 + 1 + msg_h + mode_h + 1;
        parts_x = disp_x + disp_w + 1;
        parts_y = disp_y;
        list_x = parts_x + parts_w + 1;
        list_y = disp_y;
        name_x = x0 + 1;
        name_y = disp_y + disp_h + 1;
        stats_x = x0 + 1;
        stats_y = name_y + name_h;
        req_h = disp_h;
        req_w = disp_w + parts_w + 1; // border gives another column
        req_x = disp_x;
        req_y = disp_y;

        // match grid lines
        mvwputch(w_border, h1 + 1, 0, BORDER_COLOR, LINE_XXXO); // |-
        mvwputch(w_border, h1 + 1, total_w - 1, BORDER_COLOR, LINE_XOXX); // -|
        mvwputch(w_border, h1 + 1 + disp_h + 1, 0, BORDER_COLOR, LINE_XXXO); // |-
        mvwputch(w_border, h1 + 1 + disp_h + 1, total_w - 1, BORDER_COLOR, LINE_XOXX); // -|
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
        name_w  = grid_w;
        mode_h  = 1;
        mode_w  = grid_w;
        msg_h   = 3;
        msg_w   = grid_w;

        stats_h = grid_h - mode_h - msg_h - name_h - 2;
        stats_w = 26 + (extra_w / 4);

        disp_h = stats_h / 3;
        disp_w = grid_w - stats_w - 1;
        parts_h = stats_h - disp_h - 1;
        parts_w = disp_w / 2;
        list_h = parts_h;
        list_w = disp_w - parts_w - 1;

        name_y  = y0 + 1;
        name_x  = x0 + 1;
        disp_y  = name_y + name_h + 1;
        disp_x  = x0 + 1;
        parts_y = disp_y + disp_h + 1;
        parts_x = x0 + 1;
        list_y  = parts_y;
        list_x  = x0 + 1 + parts_w + 1;
        stats_y = disp_y;
        stats_x = x0 + 1 + disp_w + 1;
        mode_y  = name_y + name_h + 1 + stats_h + 1;
        mode_x  = x0 + 1;
        msg_y   = mode_y + mode_h;
        msg_x   = x0 + 1;

        req_x = parts_x;
        req_y = parts_y;
        req_h = parts_h;
        req_w = parts_w;

        // match grid lines
        mvwputch(w_border, name_h + 1, 0, BORDER_COLOR, LINE_XXXO); // |-
        mvwputch(w_border, name_h + 1 + disp_h + 1, 0, BORDER_COLOR, LINE_XXXO); // |-
        mvwputch(w_border, name_h + 1 + stats_h + 1, 0, BORDER_COLOR, LINE_XXXO); // |-
        mvwputch(w_border, name_h + 1, total_w - 1, BORDER_COLOR, LINE_XOXX); // -|
        mvwputch(w_border, name_h + 1 + stats_h + 1, total_w - 1, BORDER_COLOR, LINE_XOXX); // -|
    }

    // make the windows
    w_mode  = newwin(mode_h,  mode_w,  mode_y,  mode_x );
    w_msg   = newwin(msg_h,   msg_w,   msg_y,   msg_x  );
    w_disp  = newwin(disp_h,  disp_w,  disp_y,  disp_x );
    w_parts = newwin(parts_h, parts_w, parts_y, parts_x);
    w_list  = newwin(list_h,  list_w,  list_y,  list_x );
    w_stats = newwin(stats_h, stats_w, stats_y, stats_x);
    w_name  = newwin(name_h,  name_w,  name_y,  name_x );
    w_req   = newwin(req_h,   req_w,   req_y,   req_x );

    page_size = list_h;

    wrefresh(w_border);
    display_grid();
    display_name();
    display_stats();
}

void veh_interact::do_main_loop()
{
    display_grid();
    display_stats ();
    display_veh   ();
    move_cursor (0, 0); // display w_disp & w_parts
    bool finish = false;
    while (!finish) {
        const std::string action = main_context.handle_input();
        int dx, dy;
        if (main_context.get_direction(dx, dy, action)) {
            move_cursor(dx, dy);
        } else if (action == "QUIT") {
            finish = true;
        } else if (action == "INSTALL") {
            do_install();
        } else if (action == "REPAIR") {
            do_repair();
        } else if (action == "REFILL") {
            do_refill();
        } else if (action == "REMOVE") {
            do_remove();
        } else if (action == "RENAME") {
            do_rename();
        } else if (action == "SIPHON") {
            do_siphon();
        } else if (action == "TIRE_CHANGE") {
            do_tirechange();
        } else if (action == "DRAIN") {
            do_drain();
        } else if (action == "RELABEL") {
            do_relabel();
        }
        if (sel_cmd != ' ') {
            finish = true;
        }
        display_mode (' ');
    }
}

void veh_interact::deallocate_windows()
{
    werase(w_grid);
    werase(w_mode);
    werase(w_msg);
    werase(w_disp);
    werase(w_parts);
    werase(w_stats);
    werase(w_list);
    werase(w_name);
    werase(w_req);
    delwin(w_grid);
    delwin(w_mode);
    delwin(w_msg);
    delwin(w_disp);
    delwin(w_parts);
    delwin(w_stats);
    delwin(w_list);
    delwin(w_name);
    delwin(w_req);
    erase();
}

void veh_interact::cache_tool_availability()
{
    crafting_inv = g->crafting_inventory(&g->u);

    int charges = dynamic_cast<it_tool *>(itypes["welder"])->charges_per_use;
    int charges_oxy = dynamic_cast<it_tool *>(itypes["oxy_torch"])->charges_per_use;
    int charges_crude = dynamic_cast<it_tool *>(itypes["welder_crude"])->charges_per_use;
    has_wrench = crafting_inv.has_tools("wrench", 1) ||
                 crafting_inv.has_tools("toolset", 1) ||
                 crafting_inv.has_tools("survivor_belt", 1) ||
                 g->u.is_wearing("survivor_belt") ||
                 crafting_inv.has_tools("toolbox", 1);
    has_hacksaw = crafting_inv.has_tools("hacksaw", 1) ||
                  crafting_inv.has_tools("survivor_belt", 1) ||
                  g->u.is_wearing("survivor_belt") ||
                  crafting_inv.has_tools("toolbox", 1) ||
                  (crafting_inv.has_tools("circsaw_off", 1) &&
                   crafting_inv.has_charges("circsaw_off", CIRC_SAW_USED)) ||
                  (crafting_inv.has_tools("oxy_torch", 1) &&
                   crafting_inv.has_charges("oxy_torch", OXY_CUTTING) &&
                  (crafting_inv.has_tools("goggles_welding", 1) ||
                   g->u.has_bionic("bio_sunglasses") ||
                   g->u.is_wearing("goggles_welding") || g->u.is_wearing("rm13_armor_on"))) ||
                  crafting_inv.has_tools("toolset", 1);
    has_welder = (crafting_inv.has_tools("welder", 1) &&
                  crafting_inv.has_charges("welder", charges)) ||
                 (crafting_inv.has_tools("oxy_torch", 1) &&
                  crafting_inv.has_charges("oxy_torch", charges_oxy)) ||
                 (crafting_inv.has_tools("welder_crude", 1) &&
                  crafting_inv.has_charges("welder_crude", charges_crude)) ||
                 (crafting_inv.has_tools("toolset", 1) &&
                  crafting_inv.has_charges("toolset", charges_crude));
    has_goggles = (crafting_inv.has_tools("goggles_welding", 1) ||
                   g->u.has_bionic("bio_sunglasses") ||
                   g->u.is_wearing("goggles_welding") || g->u.is_wearing("rm13_armor_on"));
    has_duct_tape = (crafting_inv.has_charges("duct_tape", DUCT_TAPE_USED) ||
                     (crafting_inv.has_tools("toolbox", 1) &&
                      crafting_inv.has_charges("toolbox", DUCT_TAPE_USED)));
    has_jack = crafting_inv.has_tools("jack", 1);
    has_siphon = crafting_inv.has_tools("hose", 1);

    has_wheel = crafting_inv.has_components( "wheel", 1 ) ||
                crafting_inv.has_components( "wheel_wide", 1 ) ||
                crafting_inv.has_components( "wheel_armor", 1 ) ||
                crafting_inv.has_components( "wheel_bicycle", 1 ) ||
                crafting_inv.has_components( "wheel_motorbike", 1 ) ||
                crafting_inv.has_components( "wheel_small", 1 );
}

/**
 * Checks if the player is able to perform some command, and returns a nonzero
 * error code if they are unable to perform it. The return from this function
 * should be passed into the various do_whatever functions further down.
 * @param mode The command the player is trying to perform (ie 'r' for repair).
 * @return CAN_DO if the player has everything they need,
 *         INVALID_TARGET if the command can't target that square,
 *         LACK_TOOLS if the player lacks tools,
 *         NOT_FREE if something else obstructs the action,
 *         LACK_SKILL if the player's skill isn't high enough,
 *         LOW_MORALE if the player's morale is too low while trying to perform
 *             an action requiring a minimum morale,
 *         UNKNOWN_TASK if the requested operation is unrecognized.
 */
task_reason veh_interact::cant_do (char mode)
{
    bool enough_morale = true;
    bool valid_target = false;
    bool has_tools = false;
    bool part_free = true;
    bool has_skill = true;
    bool can_remove_wheel = has_wrench && has_jack && wheel;
    bool pass_checks = false; // Used in refill only

    switch (mode) {
    case 'i': // install mode
        enough_morale = g->u.morale_level() >= MIN_MORALE_CRAFT;
        valid_target = !can_mount.empty() && 0 == veh->tags.count("convertible");
        has_tools = true; // part specific, must wait for user to select the part
        break;
    case 'r': // repair mode
        enough_morale = g->u.morale_level() >= MIN_MORALE_CRAFT;
        valid_target = !need_repair.empty() && cpart >= 0;
        has_tools = (has_welder && has_goggles) || has_duct_tape;
        break;
    case 'f': // refill mode
        if (!ptanks.empty()) {
            std::vector<vehicle_part *>::iterator iter;
            for (iter = ptanks.begin(); iter != ptanks.end(); ) {
                if ((*iter)->hp > 0 &&
                    g->refill_vehicle_part(*veh, *iter, true)) {
                    pass_checks = true;
                    iter++;
                } else {
                    iter = ptanks.erase(iter);
                }
            }
            if(pass_checks) {
                return CAN_DO;
            }
        }

        // if refillable parts exist but can't be refilled for some reason.
        if (has_ptank) {
            return CANT_REFILL;
        }

        // No refillable parts here (valid_target = false)
        break;
    case 'o': // remove mode
        enough_morale = g->u.morale_level() >= MIN_MORALE_CRAFT;
        valid_target = cpart >= 0 && 0 == veh->tags.count("convertible");
        has_tools = (has_wrench && has_hacksaw) || can_remove_wheel ||
            // UNMOUNT_ON_MOVE == loose enough to hand-remove
            (cpart >= 0 && veh->part_with_feature(cpart, "UNMOUNT_ON_MOVE") >= 0);
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
    case 'a': // relabel
        valid_target = cpart >= 0;
        has_tools = true;
        break;
    default:
        return UNKNOWN_TASK;
    }

    if( abs( veh->velocity ) > 100 || g->u.controlling_vehicle ) {
        return MOVING_VEHICLE;
    }
    if( !enough_morale ) {
        return LOW_MORALE;
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
 *               LACK_TOOLS if the player is lacking tools,
 *               LOW_MORALE if the player's morale is too low.
 */
void veh_interact::do_install()
{
    const task_reason reason = cant_do('i');
    display_mode('i');
    werase (w_msg);
    int msg_width = getmaxx(w_msg);
    switch (reason) {
    case LOW_MORALE:
        mvwprintz(w_msg, 0, 1, c_ltred, _("Your morale is too low to construct..."));
        wrefresh (w_msg);
        return;
    case INVALID_TARGET:
        mvwprintz(w_msg, 0, 1, c_ltred, _("Cannot install any part here."));
        wrefresh (w_msg);
        return;
    case MOVING_VEHICLE:
        fold_and_print( w_msg, 0, 1, msg_width - 2, c_ltgray,
                        _( "You can't install parts while driving." ) );
        wrefresh (w_msg);
        return;
    default:
        break; // no reason, all is well
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose new part to install here:"));
    wrefresh (w_mode);
    int pos = 0;
    int engines = 0;
    int dif_eng = 0;
    for (size_t p = 0; p < veh->parts.size(); p++) {
        if (veh->part_flag (p, "ENGINE")) {
            engines++;
            dif_eng = dif_eng / 2 + 12;
        }
    }
    while (true) {
        sel_vpart_info = can_mount[pos];
        const auto &req = *sel_vpart_info->installation;
        display_list (pos, can_mount);
        itype_id itm = sel_vpart_info->item;
        bool has_comps = crafting_inv.has_components(itm, 1);
        bool has_skill = g->u.skillLevel("mechanics") >= sel_vpart_info->difficulty;
        bool has_tools = req.can_make_with_inventory( crafting_inv );
        bool eng = sel_vpart_info->has_flag("ENGINE");
        bool install_pedals = sel_vpart_info->has_flag("PEDALS");
        bool install_hand_rims = sel_vpart_info->has_flag("HAND_RIMS");
        bool install_paddles = sel_vpart_info->has_flag("PADDLES");
        bool has_skill2 = !eng || (g->u.skillLevel("mechanics") >= dif_eng);
        bool has_muscle_engine = veh->has_pedals || veh->has_hand_rims || veh->has_paddles;
        bool install_muscle_engine = install_pedals || install_hand_rims || install_paddles;

        std::string engine_string = "";
        if (engines && eng) { // already has engine
            engine_string = string_format(
                                _("  You also need level <color_%1$s>%2$d</color> skill in mechanics to install additional engines."),
                                has_skill2 ? "ltgreen" : "red",
                                dif_eng);
        }
        werase (w_msg);
        werase (w_req);
        int posy = 0;
        posy += req.print( w_req, posy, 0, req_w, c_white, crafting_inv );
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("Needs level <color_%1$s>%2$d</color> skill in mechanics.%3$s"),
                       has_skill ? "ltgreen" : "red",
                       sel_vpart_info->difficulty,
                       engine_string.c_str());
        wrefresh (w_msg);
        wrefresh (w_req);
        const std::string action = main_context.handle_input();
        if ((action == "INSTALL" || action == "CONFIRM")  && has_comps && has_tools && has_skill &&
            has_skill2) {
            if (!(has_muscle_engine && eng) && !((engines > 0) && install_muscle_engine)) {
                sel_cmd = 'i';
                return;
            } else {
                werase (w_msg);
                fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltred,
                               _("That install would conflict with the existing drive system."));
                wrefresh (w_msg);
                return;
            }
        } else if (action == "QUIT") {
            werase (w_list);
            wrefresh (w_list);
            werase (w_msg);
            wrefresh(w_msg);
            move_cursor( 0, 0 );
            break;
        } else {
            move_in_list(pos, action, can_mount.size());
        }
    }
}

bool veh_interact::move_in_list(int &pos, const std::string &action, const int size) const
{
    if (action == "PREV_TAB" || action == "LEFT") {
        pos -= page_size;
    } else if (action == "NEXT_TAB" || action == "RIGHT") {
        pos += page_size;
    } else if (action == "UP") {
        pos--;
    } else if (action == "DOWN") {
        pos++;
    } else {
        // Anything else -> no movement
        return false;
    }
    if (pos < 0) {
        pos = size - 1;
    } else if (pos >= size) {
        pos = 0;
    }
    return true;
}

/**
 * Handles repairing a vehicle part.
 * @param reason INVALID_TARGET if there's no damaged parts in the selected square,
 *               LACK_TOOLS if the player is lacking tools,
 *               LOW_MORALE if the player's morale is too low.
 */
void veh_interact::do_repair()
{
    const task_reason reason = cant_do('r');
    display_mode('r');
    werase (w_msg);
    int msg_width = getmaxx(w_msg);
    switch (reason) {
    case LOW_MORALE:
        mvwprintz(w_msg, 0, 1, c_ltred, _("Your morale is too low to construct..."));
        wrefresh (w_msg);
        return;
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
                       _("You need a <color_%1$s>powered welder and welding goggles</color> or <color_%2$s>duct tape</color> to repair."),
                       (has_welder && has_goggles) ? "ltgreen" : "red",
                       has_duct_tape ? "ltgreen" : "red");
        wrefresh (w_msg);
        return;
    case MOVING_VEHICLE:
        fold_and_print( w_msg, 0, 1, msg_width - 2, c_ltgray,
                        _( "You can't repair stuff while driving." ) );
        wrefresh (w_msg);
        return;
    default:
        break; // no reason, all is well
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose a part here to repair:"));
    wrefresh (w_mode);
    int pos = 0;
    while (true) {
        sel_vehicle_part = &veh->parts[parts_here[need_repair[pos]]];
        sel_vpart_info = &sel_vehicle_part->get_vpart_info();
        werase (w_parts);
        veh->print_part_desc(w_parts, 0, parts_w, cpart, need_repair[pos]);
        wrefresh (w_parts);
        werase (w_msg);
        bool has_comps = true;
        int dif = sel_vpart_info->difficulty + ((sel_vehicle_part->hp <= 0) ? 0 : 2);
        bool has_skill = g->u.skillLevel("mechanics") >= dif;
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("You need level <color_%1$s>%2$d</color> skill in mechanics."),
                       has_skill ? "ltgreen" : "red",
                       dif);
        if (sel_vehicle_part->hp <= 0) {
            itype_id itm = sel_vpart_info->item;
            has_comps = crafting_inv.has_components(itm, 1);
            fold_and_print(w_msg, 1, 1, msg_width - 2, c_ltgray,
                           _("You also need a <color_%1$s>wrench</color> and <color_%2$s>%3$s</color> to replace broken one."),
                           has_wrench ? "ltgreen" : "red",
                           has_comps ? "ltgreen" : "red",
                           itypes[itm]->nname(1).c_str());
        }
        wrefresh (w_msg);
        const std::string action = main_context.handle_input();
        if ((action == "REPAIR" || action == "CONFIRM") &&
            has_comps &&
            (sel_vehicle_part->hp > 0 || has_wrench) && has_skill) {
            sel_cmd = 'r';
            return;
        } else if (action == "QUIT") {
            werase (w_parts);
            veh->print_part_desc (w_parts, 0, parts_w, cpart, -1);
            wrefresh (w_parts);
            werase (w_msg);
            break;
        } else {
            move_in_list(pos, action, need_repair.size());
        }
    }
}

/**
 * Handles refilling a vehicle's fuel tank.
 * @param reason INVALID_TARGET if there's no fuel tank in the spot,
 *               CANT_REFILL All tanks are broken or player has nothing to fill the tank with.
 */
void veh_interact::do_refill()
{
    const task_reason reason = cant_do('f');
    display_mode('f');
    werase (w_msg);
    int msg_width = getmaxx(w_msg);

    switch (reason) {
    case INVALID_TARGET:
        mvwprintz(w_msg, 0, 1, c_ltred, _("There's no refillable part here."));
        wrefresh (w_msg);
        return;
    case CANT_REFILL:
        mvwprintz(w_msg, 1, 1, c_ltred, _("There is no refillable part here."));
        mvwprintz(w_msg, 2, 1, c_white, _("The part is broken, or you don't have the "
                                          "proper fuel."));
        wrefresh (w_msg);
        return;
    case MOVING_VEHICLE:
        fold_and_print( w_msg, 0, 1, msg_width - 2, c_ltgray,
                        _( "You can't refill the vehicle while driving." ) );
        wrefresh (w_msg);
        return;
    default:
        break; // no reason, all is well
    }

    if (ptanks.empty()) {
        debugmsg("veh_interact::do_refill: Refillable parts list is empty.\n"
                 "veh_interact::cant_do() should control this before.");
        return;
    }
    // Now at least one of "fuel tank" is can be refilled.
    // If we have more that one tank we need to create choosing menu
    if (ptanks.size() > 1) {
        unsigned int pt_choise;
        unsigned int entry_num;
        uimenu fuel_choose;
        fuel_choose.text = _("What to refill:");
        for( entry_num = 0; entry_num < ptanks.size(); entry_num++) {
            fuel_choose.addentry(entry_num, true, -1, "%s -> %s",
                                 ammo_name(ptanks[entry_num]->get_vpart_info().fuel_type).c_str(),
                                 ptanks[entry_num]->get_vpart_info().name.c_str());
        }
        fuel_choose.addentry(entry_num, true, 'q', _("Cancel"));
        fuel_choose.query();
        pt_choise = fuel_choose.ret;
        if(pt_choise == entry_num) { // Select canceled
            return;
        }
        sel_vehicle_part = ptanks[pt_choise];
    } else {
        sel_vehicle_part = ptanks.front();
    }
    sel_cmd = 'f';
}

/**
 * Handles removing a part from the vehicle.
 * @param reason INVALID_TARGET if there are no parts to remove,
 *               LACK_TOOLS if the player is lacking tools,
 *               NOT_FREE if there's something attached that needs to be removed first,
 *               LACK_SKILL if the player's mechanics skill isn't high enough,
 *               LOW_MORALE if the player's morale is too low.
 */
void veh_interact::do_remove()
{
    const task_reason reason = cant_do('o');
    display_mode('o');
    werase (w_msg);
    int msg_width = getmaxx(w_msg);
    bool has_skill = g->u.skillLevel("mechanics") >= 2;
    bool can_hacksaw = has_wrench && has_hacksaw && has_skill;
    switch (reason) {
    case LOW_MORALE:
        mvwprintz(w_msg, 0, 1, c_ltred, _("Your morale is too low to construct..."));
        wrefresh (w_msg);
        return;
    case INVALID_TARGET:
        mvwprintz(w_msg, 0, 1, c_ltred, _("No parts here."));
        wrefresh (w_msg);
        return;
    case LACK_TOOLS:
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                       _("You need a <color_%1$s>wrench</color> and a <color_%2$s>hacksaw, cutting torch and goggles, or circular saw (off)</color> to remove parts."),
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
    case MOVING_VEHICLE:
        fold_and_print( w_msg, 0, 1, msg_width - 2, c_ltgray,
                        _( "Better not remove something will driving." ) );
        wrefresh (w_msg);
        return;
    default:
        break; // no reason, all is well
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose a part here to remove:"));
    wrefresh (w_mode);
    int first = 0;
    int pos = first;
    while (true) {
        sel_vehicle_part = &veh->parts[parts_here[pos]];
        sel_vpart_info = &sel_vehicle_part->get_vpart_info();
        bool is_wheel = sel_vpart_info->has_flag("WHEEL");
        bool is_loose = sel_vpart_info->has_flag("UNMOUNT_ON_MOVE");
        werase (w_parts);
        veh->print_part_desc (w_parts, 0, parts_w, cpart, pos);
        wrefresh (w_parts);
        const std::string action = main_context.handle_input();
        if (action == "REMOVE" || action == "CONFIRM") {
            if (veh->can_unmount(parts_here[pos])) {
                if (can_hacksaw || is_wheel || is_loose) {
                    sel_cmd = 'o';
                    return;
                } else {
                    fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                                   _("You need a <color_%1$s>wrench</color> and a <color_%2$s>hacksaw, cutting torch and goggles, or circular saw (off)</color> and <color_%3$s>level 2</color> mechanics skill to remove parts."),
                                   has_wrench ? "ltgreen" : "red",
                                   has_hacksaw ? "ltgreen" : "red",
                                   has_skill ? "ltgreen" : "red");
                    wrefresh (w_msg);
                    return;
                }
            } else {
                mvwprintz(w_msg, 0, 1, c_ltred,
                          _("You cannot remove that part while something is attached to it."));
                wrefresh (w_msg);
                return;
            }
        } else if (action == "QUIT") {
            werase (w_parts);
            veh->print_part_desc (w_parts, 0, parts_w, cpart, -1);
            wrefresh (w_parts);
            werase (w_msg);
            break;
        } else {
            move_in_list(pos, action, parts_here.size());
        }
    }
}

/**
 * Handles siphoning gas.
 * @param reason INVALID_TARGET if the vehicle has no gas,
 *               NO_TOOLS if the player has no hose.
 */
void veh_interact::do_siphon()
{
    const task_reason reason = cant_do('s');
    display_mode('s');
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
    case MOVING_VEHICLE:
        fold_and_print( w_msg, 0, 1, msg_width - 2, c_ltgray,
                        _( "You can't siphon from a moving vehicle." ) );
        wrefresh (w_msg);
        return;
    default:
        break; // no reason, all is well
    }
    sel_cmd = 's';
}

/**
 * Handles changing a tire.
 * @param reason INVALID_TARGET if there's no wheel in the selected square,
 *               LACK_TOOLS if the player is missing a tool.
 */
void veh_interact::do_tirechange()
{
    const task_reason reason = cant_do('c');
    display_mode('c');
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
    case MOVING_VEHICLE:
        fold_and_print( w_msg, 0, 1, msg_width - 2, c_ltgray, _( "Who is driving while you work?" ) );
        wrefresh (w_msg);
        return;
    default:
        break; // no reason, all is well
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose wheel to use as replacement:"));
    wrefresh (w_mode);
    int pos = 0;
    while (true) {
        sel_vpart_info = wheel_types[pos];
        bool is_wheel = sel_vpart_info->has_flag("WHEEL");
        display_list (pos, wheel_types);
        itype_id itm = sel_vpart_info->item;
        bool has_comps = crafting_inv.has_components(itm, 1);
        bool has_tools = has_jack && has_wrench;
        werase (w_msg);
        wrefresh (w_msg);
        const std::string action = main_context.handle_input();
        if ((action == "TIRE_CHANGE" || action == "CONFIRM") && has_comps && has_tools && is_wheel) {
            sel_cmd = 'c';
            return;
        } else if (action == "QUIT") {
            werase (w_list);
            wrefresh (w_list);
            werase (w_msg);
            break;
        } else {
            move_in_list(pos, action, wheel_types.size());
        }
    }
}

/**
 * Handles draining water from a vehicle.
 * @param reason INVALID_TARGET if the vehicle has no water,
 *               LACK_TOOLS if the player has no hose.
 */
void veh_interact::do_drain()
{
    const task_reason reason = cant_do('d');
    display_mode('d');
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
    case MOVING_VEHICLE:
        fold_and_print( w_msg, 0, 1, msg_width - 2, c_ltgray,
                        _( "You can't siphon from a moving vehicle." ) );
        wrefresh (w_msg);
        return;
    default:
        break; // no reason, all is well
    }
    sel_cmd = 'd';
}

/**
 * Handles renaming a vehicle.
 * @param reason Unused.
 */
void veh_interact::do_rename()
{
    display_mode('e');
    std::string name = string_input_popup(_("Enter new vehicle name:"), 20);
    if(name.length() > 0) {
        (veh->name = name);
        if (veh->tracking_on) {
            overmap_buffer.remove_vehicle( veh );
            // Add the vehicle again, this time with the new name
            overmap_buffer.add_vehicle( veh );
        }
    }
    display_grid();
    display_name();
    display_stats();
    // refresh w_disp & w_part windows:
    move_cursor(0, 0);
}

/**
 * Relabels the currently selected square.
 */
void veh_interact::do_relabel()
{
    display_mode('a');
    const task_reason reason = cant_do('a');
    if (reason == INVALID_TARGET) {
        mvwprintz(w_msg, 0, 1, c_ltred, _("There are no parts here to label."));
        wrefresh (w_msg);
        return;
    }
    std::string text = string_input_popup(_("New label:"), 20, veh->get_label(-ddx, -ddy));
    veh->set_label(-ddx, -ddy, text); // empty input removes the label
    display_grid();
    display_name();
    display_stats();
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
 * Checks to see if you can currently install this part at current position.
 * Affects coloring in display_list() and is also used to
 * sort can_mount so installable parts come first.
 */
bool veh_interact::can_currently_install(const vpart_info *vpart)
{
    bool has_comps = crafting_inv.has_components(vpart->item, 1);
    bool has_skill = g->u.skillLevel("mechanics") >= vpart->difficulty;
    bool is_wheel = vpart->has_flag("WHEEL");
    return (has_comps && (has_skill || is_wheel));
}

/**
 * Moves the cursor on the vehicle editing window.
 * @param dx How far to move the cursor on the x-axis.
 * @param dy How far to move the cursor on the y-axis.
 */
void veh_interact::move_cursor (int dx, int dy)
{
    const int hw = getmaxx(w_disp) / 2;
    const int hh = getmaxy(w_disp) / 2;

    if (vertical_menu) {
        ddx += dy;
        ddy -= dx;
    } else {
        ddx -= dx;
        ddy -= dy;
    }
    display_veh();
    // Update the current active component index to the new position.
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
    long sym = cpart >= 0 ? veh->part_sym( cpart ) : ' ';
    if( !vertical_menu ) {
        // Rotate the symbol if necessary.
        tileray tdir( 0 );
        sym = tdir.dir_symbol( sym );
    }
    mvwputch (w_disp, hh, hw, obstruct ? red_background(col) : hilite(col),
              special_symbol(sym));
    wrefresh (w_disp);
    werase (w_parts);
    veh->print_part_desc (w_parts, 0, parts_w, cpart, -1);
    wrefresh (w_parts);

    can_mount.clear();
    if (!obstruct) {
        int divider_index = 0;
        for (std::map<std::string, vpart_info>::iterator
             part_type_iterator = vehicle_part_types.begin();
             part_type_iterator != vehicle_part_types.end();
             ++part_type_iterator) {
            if (veh->can_mount(vdx, vdy, part_type_iterator->first)) {
                const vpart_info *vpi = &part_type_iterator->second;
                if (can_currently_install(vpi)) {
                    can_mount.insert( can_mount.begin() + divider_index++, vpi );
                } else {
                    can_mount.push_back( vpi );
                }
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
                wheel_types.push_back (&part_type_iterator->second);
            }
        }
    }

    need_repair.clear();
    parts_here.clear();
    ptanks.clear();
    has_ptank = false;
    wheel = NULL;
    if (cpart >= 0) {
        parts_here = veh->parts_at_relative(veh->parts[cpart].mount_dx, veh->parts[cpart].mount_dy);
        for (size_t i = 0; i < parts_here.size(); i++) {
            int p = parts_here[i];
            if (veh->parts[p].hp < veh->part_info(p).durability) {
                need_repair.push_back (i);
            }
            if (veh->part_flag(p, "FUEL_TANK") && veh->parts[p].amount < veh->part_info(p).size) {
                ptanks.push_back(&veh->parts[p]);
                has_ptank = true;
            }
            if (veh->part_flag(p, "WHEEL")) {
                wheel = &veh->parts[p];
            }
        }
    }
    werase (w_msg);
    wrefresh (w_msg);
    display_mode (' ');
}

void veh_interact::display_grid()
{
    const int grid_w = getmaxx(w_grid);
    if (vertical_menu) {
        // Two lines dividing the three middle sections.
        for (int i = 1 + mode_h + msg_h; i < (1 + mode_h + msg_h + disp_h); ++i) {
            mvwputch(w_grid, i, disp_w, BORDER_COLOR, LINE_XOXO); // |
            mvwputch(w_grid, i, disp_w + 1 + parts_w, BORDER_COLOR, LINE_XOXO); // |
        }
        // Two lines dividing the vertical menu sections.
        for (int i = 0; i < grid_w; ++i) {
            mvwputch( w_grid, mode_h + msg_h, i, BORDER_COLOR, LINE_OXOX ); // -
            mvwputch( w_grid, mode_h + msg_h + 1 + disp_h, i, BORDER_COLOR, LINE_OXOX ); // -
        }
        // Fix up the line intersections.
        mvwputch(w_grid, mode_h + msg_h,              disp_w, BORDER_COLOR, LINE_OXXX);
        mvwputch(w_grid, mode_h + msg_h + 1 + disp_h, disp_w, BORDER_COLOR, LINE_XXOX); // _|_
        mvwputch(w_grid, mode_h + msg_h,              disp_w + 1 + parts_w, BORDER_COLOR, LINE_OXXX);
        mvwputch(w_grid, mode_h + msg_h + 1 + disp_h, disp_w + 1 + parts_w, BORDER_COLOR, LINE_XXOX); // _|_
    } else {
        // Vertical lines
        mvwvline(w_grid, name_h + 1, disp_w, LINE_XOXO, disp_h + 1 + parts_h);
        mvwvline(w_grid, name_h + 1 + disp_h + 1, parts_w, LINE_XOXO, (stats_h - disp_h - 1));
        // Two horizontal lines: one after name window, and another after parts window
        mvwhline(w_grid, name_h, 0, LINE_OXOX, grid_w);
        mvwhline(w_grid, name_h + 1 + stats_h, 0, LINE_OXOX, grid_w);
        // Horizontal line between vehicle/parts windows
        mvwhline(w_grid, name_h + 1 + disp_h, 0, LINE_OXOX, disp_w);
        // Fix up the line intersections.
        mvwputch(w_grid, name_h, disp_w, BORDER_COLOR, LINE_OXXX);
        mvwputch(w_grid, name_h + 1 + disp_h, parts_w, BORDER_COLOR, LINE_OXXX);
        mvwputch(w_grid, name_h + 1 + disp_h + parts_h + 1, parts_w, BORDER_COLOR, LINE_XXOX);
        mvwputch(w_grid, name_h + 1 + disp_h, disp_w, BORDER_COLOR, LINE_XOXX); // -|
        mvwputch(w_grid, name_h + 1 + stats_h, disp_w, BORDER_COLOR, LINE_XXOX );
    }
    wrefresh(w_grid);
}

/**
 * Draws the viewport with the vehicle.
 */
void veh_interact::display_veh ()
{
    werase(w_disp);
    const int hw = getmaxx(w_disp) / 2;
    const int hh = getmaxy(w_disp) / 2;
    //Iterate over structural parts so we only hit each square once
    std::vector<int> structural_parts = veh->all_parts_at_location("structure");
    int x, y;
    for (size_t i = 0; i < structural_parts.size(); i++) {
        const int p = structural_parts[i];
        long sym = veh->part_sym (p);
        nc_color col = veh->part_color (p);
        if (vertical_menu) {
            x =   veh->parts[p].mount_dy + ddy;
            y = -(veh->parts[p].mount_dx + ddx);
        } else {
            tileray tdir( 0 );
            sym = tdir.dir_symbol( sym );
            x = veh->parts[p].mount_dx + ddx;
            y = veh->parts[p].mount_dy + ddy;
        }
        if (x == 0 && y == 0) {
            col = hilite(col);
            cpart = p;
        }
        mvwputch (w_disp, hh + y, hw + x, col, special_symbol(sym));
    }
    if (!vertical_menu) {
        size_t len = utf8_width(_("FWD ->"));
        mvwprintz(w_disp, 0, disp_w - len, c_dkgray,  _("FWD ->"));
    }
    wrefresh (w_disp);
}

/**
 * Displays the vehicle's stats at the bottom of the window.
 */
void veh_interact::display_stats()
{
    const int extraw = ((TERMX - FULL_SCREEN_WIDTH) / 4) * 2; // see exec()
    int x[15], y[15], w[15]; // 3 columns * 5 rows = 15 slots max

    std::vector<int> cargo_parts = veh->all_parts_with_feature("CARGO");
    int total_cargo = 0;
    int free_cargo = 0;
    for (size_t i = 0; i < cargo_parts.size(); i++) {
        const int p = cargo_parts[i];
        total_cargo += veh->max_volume(p);
        free_cargo += veh->free_volume(p);
    }
    if (vertical_menu) {
        // Vertical menu
        const int second_column = 34 + (extraw / 4); // 29
        const int third_column = 63 + (extraw / 2);  // 56
        for (int i = 0; i < 15; i++) {
            if (i < 5) { // First column
                x[i] = 1;
                y[i] = i;
                w[i] = second_column - 2;
            } else if (i < 10) { // Second column
                x[i] = second_column;
                y[i] = i - 5;
                w[i] = third_column - second_column - 1;
            } else { // Third column
                x[i] = third_column;
                y[i] = i - 10;
                w[i] = extraw - third_column - 2;
            }
        }
    } else {
        for (int i = 0; i < 15; i++) {
            x[i] = 1;
            y[i] = i;
            w[i] = stats_w - 1;
        }
    }

    bool isBoat = !veh->all_parts_with_feature(VPFLAG_FLOATS).empty();
    bool conf;

    conf = veh->valid_wheel_config();

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
    fold_and_print(w_stats, y[0], x[0], w[0], c_ltgray,
                   _("Safe/Top speed: <color_ltgreen>%3d</color>/<color_ltred>%3d</color> %s"),
                   int(veh->safe_velocity(false) * speed_factor),
                   int(veh->max_velocity(false) * speed_factor), speed_units.c_str());
    fold_and_print(w_stats, y[1], x[1], w[1], c_ltgray,
                   _("Acceleration: <color_ltblue>%3d</color> %s/t"),
                   int(veh->acceleration(false) * speed_factor), speed_units.c_str());
    fold_and_print(w_stats, y[2], x[2], w[2], c_ltgray,
                   _("Mass: <color_ltblue>%5d</color> %s"),
                   int(veh->total_mass() * weight_factor), weight_units.c_str());
    fold_and_print(w_stats, y[3], x[3], w[3], c_ltgray,
                   _("Cargo Volume: <color_ltgray>%d/%d</color>"),
                   total_cargo - free_cargo, total_cargo);
    // Write the overall damage
    mvwprintz(w_stats, y[4], x[4], c_ltgray, _("Status:  "));
    x[4] += utf8_width(_("Status: ")) + 1;
    fold_and_print(w_stats, y[4], x[4], w[4], totalDurabilityColor, totalDurabilityText);

    if( !isBoat ) {
        if( conf ) {
            fold_and_print(w_stats, y[5], x[5], w[5], c_ltgray,
                           _("Wheels:    <color_ltgreen>enough</color>"));
        }   else {
            fold_and_print(w_stats, y[5], x[5], w[5], c_ltgray,
                           _("Wheels:      <color_ltred>lack</color>"));
        }
    }   else {
        if (conf) {
            fold_and_print(w_stats, y[5], x[5], w[5], c_ltgray,
                           _("Boat:    <color_blue>can swim</color>"));
        }   else {
            fold_and_print(w_stats, y[5], x[5], w[5], c_ltgray,
                           _("Boat:  <color_ltred>can't swim</color>"));
        }
    }

    // Write the most damaged part
    if (mostDamagedPart != -1) {
        std::string partName;
        mvwprintz(w_stats, y[6], x[6], c_ltgray, _("Most damaged: "));
        x[6] += utf8_width(_("Most damaged: ")) + 1;
        const vehicle_part &part = veh->parts[mostDamagedPart];
        int damagepercent = 100 * part.hp / part.get_vpart_info().durability;
        nc_color damagecolor = getDurabilityColor(damagepercent);
        partName = part.get_vpart_info().name;
        fold_and_print(w_stats, y[6], x[6], w[6], damagecolor, partName);
    }

    fold_and_print(w_stats, y[7], x[7], w[7], c_ltgray,
                   _("K dynamics:   <color_ltblue>%3d</color>%%"),
                   int(veh->k_dynamics() * 100));
    fold_and_print(w_stats, y[8], x[8], w[8], c_ltgray,
                   _("K mass:       <color_ltblue>%3d</color>%%"),
                   int(veh->k_mass() * 100));

    // "Fuel usage (safe): " is renamed to "Fuel usage: ".
    mvwprintz(w_stats, y[9], x[9], c_ltgray,  _("Fuel usage:     "));
    x[9] += utf8_width(_("Fuel usage:     "));
    ammotype fuel_types[3] = { "gasoline", "battery", "plasma" };
    nc_color fuel_colors[3] = { c_ltred, c_yellow, c_ltblue };
    bool first = true;
    int fuel_name_length = 0;
    for (int i = 0; i < 3; ++i) {
        int fuel_usage = veh->basic_consumption(fuel_types[i]);
        if (fuel_usage > 0) {
            fuel_name_length = std::max(fuel_name_length, utf8_width(ammo_name(fuel_types[i]).c_str()));
            fuel_usage = fuel_usage / 100;
            if (fuel_usage < 1) {
                fuel_usage = 1;
            }
            if (!first) {
                mvwprintz(w_stats, y[9], x[9]++, c_ltgray, "/");
            }
            mvwprintz(w_stats, y[9], x[9]++, fuel_colors[i], "%d", fuel_usage);
            if (fuel_usage > 9) {
                x[9]++;
            }
            if (fuel_usage > 99) {
                x[9]++;
            }
            first = false;
        }
        if (first) {
            mvwprintz(w_stats, y[9], x[9], c_ltgray, "-"); // no engines
        }
    }

    // Print fuel percentage & type name only if it fits in the window, 13 is width of "E...F 100% - "
    veh->print_fuel_indicator (w_stats, y[10], x[10], true,
                               (x[10] + 13 < stats_w),
                               (x[10] + 13 + fuel_name_length < stats_w));

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
        esc_pos = getmaxx(w_mode);
    }

    if (mode == ' ') {
        std::vector<std::string> actions;
        actions.push_back(_("<i>nstall"));
        actions.push_back(_("<r>epair"));
        actions.push_back(_("re<f>ill"));
        actions.push_back(_("rem<o>ve"));
        actions.push_back(_("<s>iphon"));
        actions.push_back(_("<d>rain"));
        actions.push_back(_("<c>hange tire"));
        actions.push_back(_("r<e>name"));
        actions.push_back(_("l<a>bel"));

        bool enabled[9];
        enabled[0] = !cant_do('i');
        enabled[1] = !cant_do('r');
        enabled[2] = !cant_do('f');
        enabled[3] = !cant_do('o');
        enabled[4] = !cant_do('s');
        enabled[5] = !cant_do('d');
        enabled[6] = !cant_do('c');
        enabled[7] = true;          // 'rename' is always available
        enabled[8] = !cant_do('a');

        int pos[10];
        pos[0] = 1;
        for (size_t i = 0; i < actions.size(); i++) {
            pos[i + 1] = pos[i] + utf8_width(actions[i].c_str()) - 2;
        }
        int spacing = int((esc_pos - 1 - pos[actions.size()]) / actions.size());
        int shift = int((esc_pos - pos[actions.size()] - spacing * (actions.size() - 1)) / 2) - 1;
        for (size_t i = 0; i < actions.size(); i++) {
            shortcut_print(w_mode, 0, pos[i] + spacing * i + shift,
                           enabled[i] ? c_ltgray : c_dkgray, enabled[i] ? c_ltgreen : c_green,
                           actions[i]);
        }
    }
    wrefresh (w_mode);
}

size_t veh_interact::display_esc(WINDOW *win)
{
    std::string backstr = _("<ESC>-back");
    size_t pos = getmaxx(win) - utf8_width(backstr.c_str()) + 2;    // right text align
    shortcut_print(win, 0, pos, c_ltgray, c_ltgreen, backstr);
    wrefresh(win);
    return pos;
}

/**
 * Draws the list of parts that can be mounted in the selected square. Used
 * when installing new parts or changing tires.
 * @param pos The current cursor position in the list.
 * @param list The list to display parts from.
 */
void veh_interact::display_list(size_t pos, std::vector<const vpart_info*> list)
{
    werase (w_list);
    size_t page = pos / page_size;
    for (size_t i = page * page_size; i < (page + 1) * page_size && i < list.size(); i++) {
        int y = i - page * page_size;
        nc_color col = can_currently_install(list[i]) ? c_white : c_dkgray;
        mvwprintz(w_list, y, 3, pos == i ? hilite (col) : col, list[i]->name.c_str());
        mvwputch (w_list, y, 1, list[i]->color, special_symbol(list[i]->sym));
    }
    wrefresh (w_list);
}

void veh_interact::countDurability()
{
    int sum = 0; // sum of part HP
    int max = 0; // sum of part max HP, i.e. durability
    double mostDamaged = 1; // durability ratio of the most damaged part

    for (size_t it = 0; it < veh->parts.size(); it++) {
        if (veh->parts[it].removed) {
            continue;
        }
        vehicle_part part = veh->parts[it];
        int part_dur = part.get_vpart_info().durability;

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
    durabilityPercent = int(totalDamagePercent * 100);

    totalDurabilityColor = getDurabilityColor(durabilityPercent);
    totalDurabilityText = getDurabilityDescription(durabilityPercent);
}

nc_color getDurabilityColor(const int &dur)
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

/**
 * Given a vpart id, gives the choice of inventory and nearby items to consume
 * for install/repair/etc. Doesn't use consume_items in crafting.cpp, as it got
 * into weird cases and doesn't consider properties like HP and bigness. The
 * item will be removed by this function.
 * @param vpid The id of the vpart type to look for.
 * @return The item that was consumed.
 */
item consume_vpart_item (std::string vpid)
{
    std::vector<bool> candidates;
    const itype_id itid = vehicle_part_types[vpid].item;
    inventory map_inv;
    map_inv.form_from_map( point(g->u.posx, g->u.posy), PICKUP_RANGE );

    if( g->u.has_amount( itid, 1 ) ) {
        candidates.push_back( true );
    }
    if( map_inv.has_components( itid, 1 ) ) {
        candidates.push_back( false );
    }

    // bug?
    if(candidates.empty()) {
        debugmsg("Part not found!");
        return item();
    }

    int selection;
    // no choice?
    if(candidates.size() == 1) {
        selection = 0;
    } else {
        // popup menu!?
        std::vector<std::string> options;
        for( size_t i = 0; i < candidates.size(); ++i ) {
            if( candidates[i] ) {
                // In inventory.
                options.push_back(vehicle_part_types[vpid].name);
            } else {
                // Nearby.
                options.push_back(vehicle_part_types[vpid].name + _(" (nearby)"));
            }
        }
        selection = menu_vec(false, _("Use which gizmo?"), options);
        selection -= 1;
    }
    std::list<item> item_used;
    //remove item from inventory. or map.
    if( candidates[selection] ) {
        item_used = g->u.use_amount( itid, 1 );
    } else {
        item_used = g->m.use_amount( point(g->u.posx, g->u.posy), PICKUP_RANGE, itid, 1 );
    }

    return item_used.front();
}

/**
 * Called when the activity timer for installing parts, repairing, etc times
 * out and the the action is complete.
 */
void complete_vehicle ()
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
    std::vector<tool_comp> tools;
    int welder_charges = dynamic_cast<it_tool *>(itypes["welder"])->charges_per_use;
    int welder_oxy_charges = dynamic_cast<it_tool *>(itypes["oxy_torch"])->charges_per_use;
    int welder_crude_charges = dynamic_cast<it_tool *>(itypes["welder_crude"])->charges_per_use;
    inventory crafting_inv = g->crafting_inventory(&g->u);
    int partnum;
    item used_item;
    bool broken;
    int replaced_wheel;
    std::vector<int> parts;
    int dd = 2;
    double dmg = 1.0;

    // For siphoning from adjacent vehicles
    int posx = 0;
    int posy = 0;
    std::map<point, vehicle *> foundv;
    vehicle *fillv = NULL;
    const vpart_info *sel_vpart_info = nullptr;

    // cmd = Install Repair reFill remOve Siphon Drainwater Changetire reName relAbel
    switch (cmd) {
    case 'i':
        sel_vpart_info = &vehicle_part_types[part_id];
        sel_vpart_info->installation->use_components_and_tools( g->u, crafting_inv );

        used_item = consume_vpart_item (part_id);
        partnum = veh->install_part (dx, dy, part_id, used_item);
        if(partnum < 0) {
            debugmsg ("complete_vehicle install part fails dx=%d dy=%d id=%d", dx, dy, part_id.c_str());
        }

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

        add_msg (m_good, _("You install a %s into the %s."),
                 vehicle_part_types[part_id].name.c_str(), veh->name.c_str());
        g->u.practice( "mechanics", vehicle_part_types[part_id].difficulty * 5 + 20 );
        break;
    case 'r':
        veh->last_repair_turn = calendar::turn;
        if (veh->parts[vehicle_part].hp <= 0) {
            veh->break_part_into_pieces(vehicle_part, g->u.posx, g->u.posy);
            used_item = consume_vpart_item (veh->parts[vehicle_part].get_id());
            veh->parts[vehicle_part].bigness = used_item.bigness;
            tools.push_back(tool_comp("wrench", -1));
            tools.push_back(tool_comp("survivor_belt", -1));
            tools.push_back(tool_comp("toolbox", -1));
            g->consume_tools(&g->u, tools);
            tools.clear();
            dd = 0;
            veh->insides_dirty = true;
        } else {
            dmg = 1.1 - double(veh->parts[vehicle_part].hp) / veh->part_info(vehicle_part).durability;
        }
        tools.push_back(tool_comp("welder", int(welder_charges * dmg)));
        tools.push_back(tool_comp("oxy_torch", int(welder_oxy_charges * dmg)));
        tools.push_back(tool_comp("welder_crude", int(welder_crude_charges * dmg)));
        tools.push_back(tool_comp("duct_tape", int(DUCT_TAPE_USED * dmg)));
        tools.push_back(tool_comp("toolbox", int(DUCT_TAPE_USED * dmg)));
        tools.push_back(tool_comp("toolset", int(welder_crude_charges * dmg)));
        g->consume_tools(&g->u, tools);
        veh->parts[vehicle_part].hp = veh->part_info(vehicle_part).durability;
        add_msg (m_good, _("You repair the %s's %s."),
                 veh->name.c_str(), veh->part_info(vehicle_part).name.c_str());
        g->u.practice( "mechanics", int(((veh->part_info(vehicle_part).difficulty + dd) * 5 + 20)*dmg) );
        break;
    case 'f':
        if (!g->pl_refill_vehicle(*veh, vehicle_part, true)) {
            debugmsg ("complete_vehicle refill broken");
        }
        g->pl_refill_vehicle(*veh, vehicle_part);
        break;
    case 'o':
        // Loose part removal (e.g. jumper cables) is a tool-free action.
        if (!veh->part_flag(vehicle_part, "UNMOUNT_ON_MOVE")) {
            tools.push_back(tool_comp("hacksaw", -1));
            tools.push_back(tool_comp("toolbox", -1));
            tools.push_back(tool_comp("survivor_belt", -1));
            tools.push_back(tool_comp("circsaw_off", 20));
            tools.push_back(tool_comp("oxy_torch", 10));
            g->consume_tools(&g->u, tools);
        }
        // Dump contents of part at player's feet, if any.
        for (size_t i = 0; i < veh->parts[vehicle_part].items.size(); i++) {
            g->m.add_item_or_charges (g->u.posx, g->u.posy, veh->parts[vehicle_part].items[i]);
        }
        veh->parts[vehicle_part].items.clear();

        // Power cables must remove parts from the target vehicle, too.
        if (veh->part_flag(vehicle_part, "POWER_TRANSFER")) {
            veh->remove_remote_part(vehicle_part);
        }

        broken = veh->parts[vehicle_part].hp <= 0;
        if (!broken) {
            used_item = veh->parts[vehicle_part].properties_to_item();
            g->m.add_item_or_charges(g->u.posx, g->u.posy, used_item);
            if(type != SEL_JACK) { // Changing tires won't make you a car mechanic
                g->u.practice ( "mechanics", 2 * 5 + 20 );
            }
        } else {
            veh->break_part_into_pieces(vehicle_part, g->u.posx, g->u.posy);
        }
        if (veh->parts.size() < 2) {
            add_msg (_("You completely dismantle the %s."), veh->name.c_str());
            g->u.activity.type = ACT_NULL;
            g->m.destroy_vehicle (veh);
        } else {
            if (broken) {
                add_msg(_("You remove the broken %s from the %s."),
                        veh->part_info(vehicle_part).name.c_str(),
                        veh->name.c_str());
            } else {
                add_msg(_("You remove the %s from the %s."),
                        veh->part_info(vehicle_part).name.c_str(),
                        veh->name.c_str());
            }
            veh->remove_part (vehicle_part);
            veh->part_removal_cleanup();
        }
        break;
    case 's':

        for (int x = g->u.posx - 1; x < g->u.posx + 2; x++) {
            for (int y = g->u.posy - 1; y < g->u.posy + 2; y++) {
                fillv = g->m.veh_at(x, y);
                if ( fillv != NULL &&
                     fillv != veh &&
                     foundv.find( point(fillv->posx, fillv->posy) ) == foundv.end() &&
                     fillv->fuel_capacity("gasoline") > 0 ) {
                    foundv[point(fillv->posx, fillv->posy)] = fillv;
                }
            }
        }
        fillv = NULL;
        if ( ! foundv.empty() ) {
            uimenu fmenu;
            fmenu.text = _("Fill what?");
            fmenu.addentry(_("Nearby vehicle (%d)"), foundv.size());
            fmenu.addentry(_("Container"));
            fmenu.addentry(_("Never mind"));
            fmenu.query();
            if ( fmenu.ret == 0 ) {
                if ( foundv.size() > 1 ) {
                    if(choose_adjacent(_("Fill which vehicle?"), posx, posy)) {
                        fillv = g->m.veh_at(posx, posy);
                    } else {
                        break;
                    }
                } else {
                    fillv = foundv.begin()->second;

                }
            } else if ( fmenu.ret != 1 ) {
                break;
            }
        }
        if ( fillv != NULL ) {
            int want = fillv->fuel_capacity("gasoline") - fillv->fuel_left("gasoline");
            int got = veh->drain("gasoline", want);
            fillv->refill("gasoline", got);
            add_msg(ngettext("Siphoned %d unit of %s from the %s into the %s%s",
                             "Siphoned %d units of %s from the %s into the %s%s",
                             got),
                    got, "gasoline", veh->name.c_str(), fillv->name.c_str(),
                    (got < want ? ", draining the tank completely." : ", receiving tank is full.") );
            g->u.moves -= 200;
        } else {
            g->u.siphon( veh, "gasoline" );
        }
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
            removed_wheel = veh->parts[replaced_wheel].properties_to_item();
            veh->remove_part( replaced_wheel );
            veh->part_removal_cleanup();
            add_msg( _("You replace one of the %s's tires with a %s."),
                     veh->name.c_str(), vehicle_part_types[part_id].name.c_str() );
            used_item = consume_vpart_item( part_id );
            partnum = veh->install_part( dx, dy, part_id, used_item );
            if( partnum < 0 ) {
                debugmsg ("complete_vehicle tire change fails dx=%d dy=%d id=%d", dx, dy, part_id.c_str());
            }
            // Place the removed wheel on the map last so consume_vpart_item() doesn't pick it.
            if ( !broken ) {
                g->m.add_item_or_charges( g->u.posx, g->u.posy, removed_wheel );
            }
        }
        break;
    case 'd':
        g->u.siphon( veh, "water" );
        break;
    }
}
