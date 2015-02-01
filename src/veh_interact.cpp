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
    int stats_x, stats_y, list_x, list_y, name_x, name_y;

    if (vertical_menu) {
        //         Vertical menu:
        // +----------------------------+
        // |           w_mode           |
        // |           w_msg            |
        // +--------+---------+---------+
        // | w_disp | w_parts |  w_list |
        // +--------+---------+---------+
        // |          w_name   w_details|
        // |          w_stats           |
        // +----------------------------+
        //
        // w_disp/w_parts/w_list expand to take up extra height.
        // w_disp, w_parts and w_list share extra width in a 2:1:1 ratio,
        // but w_parts and w_list start with more than w_disp.
        //
        // w_details only shows in install view and covers rightmost colums of w_name and w_stats

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
        // | w_parts | w_list |w_details|
        // +---------+--------+---------+
        // |           w_mode           |
        // |           w_msg            |
        // +----------------------------+
        //
        // w_details only shows in install view and covers lower part of w_stats

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
    w_details = NULL;  // only pops up when in install menu
    w_stats = newwin(stats_h, stats_w, stats_y, stats_x);
    w_name  = newwin(name_h,  name_w,  name_y,  name_x );

    page_size = list_h;

    wrefresh(w_border);
    delwin( w_border );
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
 * itype::charges_per_use of a tool (itype of given id)
 */
static int charges_per_use( const std::string &id )
{
    const it_tool *t = dynamic_cast<const it_tool *>( item::find_type( id ) );
    if( t == nullptr ) {
        debugmsg( "item %s is not a tool as expected", id.c_str() );
        return 0;
    }
    return t->charges_per_use;
}

void veh_interact::cache_tool_availability()
{
    crafting_inv = g->u.crafting_inventory();

    int charges = charges_per_use( "welder" );
    int charges_oxy = charges_per_use( "oxy_torch" );
    int charges_crude = charges_per_use( "welder_crude" );
    has_wrench = crafting_inv.has_items_with_quality( "WRENCH", 1, 1 );
    has_hammer = crafting_inv.has_items_with_quality( "HAMMER", 1, 1 );
    has_nailgun = crafting_inv.has_tools("nailgun", 1);
    has_hacksaw = crafting_inv.has_items_with_quality( "SAW_M_FINE", 1, 1 ) ||
                  (crafting_inv.has_tools("circsaw_off", 1) &&
                   crafting_inv.has_charges("circsaw_off", CIRC_SAW_USED)) ||
                  (crafting_inv.has_tools("oxy_torch", 1) &&
                   crafting_inv.has_charges("oxy_torch", OXY_CUTTING) &&
                  (crafting_inv.has_tools("goggles_welding", 1) ||
                   g->u.has_bionic("bio_sunglasses") ||
                   g->u.is_wearing("goggles_welding") || g->u.is_wearing("rm13_armor_on")));
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
    has_nails = crafting_inv.has_charges("nail", NAILS_USED);
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
    bool pass_checks = false; // Used in refill only

    switch (mode) {
    case 'i': // install mode
        enough_morale = g->u.morale_level() >= MIN_MORALE_CRAFT;
        valid_target = !can_mount.empty() && 0 == veh->tags.count("convertible");
        //tool checks processed later
        has_tools = true;
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
        part_free = parts_here.size() > 1 || (cpart >= 0 && veh->can_unmount(cpart));
        //tool and skill checks processed later
        has_tools = true;
        has_skill = true;
        break;
    case 's': // siphon mode
        valid_target = (veh->fuel_left("gasoline") > 0 || veh->fuel_left("diesel") > 0);
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

bool veh_interact::is_drive_conflict(int msg_width){
    bool install_muscle_engine = (sel_vpart_info->fuel_type == "muscle");
    bool has_muscle_engine = veh->has_engine_type("muscle", false);
    bool can_install = !(has_muscle_engine && install_muscle_engine);

    if (!can_install) {
        werase (w_msg);
        fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltred,
                       _("Only one muscle powered engine can be installed."));
        wrefresh (w_msg);
    }
    return !can_install;
}

bool veh_interact::can_install_part(int msg_width){
    if( sel_vpart_info == NULL ) {
        werase (w_msg);
        wrefresh (w_msg);
        return false;
    }

    bool is_engine = sel_vpart_info->has_flag("ENGINE");
    bool install_muscle_engine = (sel_vpart_info->fuel_type == "muscle");
    //count current engines, muscle engines don't require higher skill
    int engines = 0;
    int dif_eng = 0;
    if (is_engine && !install_muscle_engine) {
        for (size_t p = 0; p < veh->parts.size(); p++) {
            if (veh->part_flag (p, "ENGINE") &&
                veh->part_info(p).fuel_type != "muscle")
            {
                engines++;
                dif_eng = dif_eng / 2 + 12;
            }
        }
    }

    itype_id itm = sel_vpart_info->item;
    bool drive_conflict = is_drive_conflict(msg_width);

    bool has_comps = crafting_inv.has_components(itm, 1);
    bool has_skill = g->u.skillLevel("mechanics") >= sel_vpart_info->difficulty;
    bool has_tools = ((has_welder && has_goggles) || has_duct_tape) && has_wrench;
    bool has_skill2 = !is_engine || (g->u.skillLevel("mechanics") >= dif_eng);
    bool is_wrenchable = sel_vpart_info->has_flag("TOOL_WRENCH");
    bool is_wood = sel_vpart_info->has_flag("NAILABLE");
    bool is_hand_remove = sel_vpart_info->has_flag("TOOL_NONE");
    std::string engine_string = "";
    if (!drive_conflict){
        if (engines && is_engine) { // already has engine
            engine_string = string_format(
                                _("  You also need level <color_%1$s>%2$d</color> skill in mechanics to install additional engines."),
                                has_skill2 ? "ltgreen" : "red",
                                dif_eng);
        }
        if (is_wood) {
            werase (w_msg);
            fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                           _("Needs <color_%1$s>%2$s</color>, a <color_%3$s>hammer</color> or <color_%4$s>nailgun</color>, <color_%5$s>nails</color> and level <color_%6$s>%7$d</color> skill in mechanics.%8$s"),
                           has_comps ? "ltgreen" : "red",
                           item::nname( itm ).c_str(),
                           has_hammer ? "ltgreen" : "red",
                           has_nailgun ? "ltgreen" : "red",
                           has_nails ? "ltgreen" : "red",
                           has_skill ? "ltgreen" : "red",
                           sel_vpart_info->difficulty,
                           engine_string.c_str());
            wrefresh (w_msg);
        } else if (is_wrenchable){
            werase (w_msg);
            fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                           _("Needs <color_%1$s>%2$s</color>, a <color_%3$s>wrench</color> and level <color_%4$s>%5$d</color> skill in mechanics.%6$s"),
                           has_comps ? "ltgreen" : "red",
                           item::nname( itm ).c_str(),
                           has_wrench ? "ltgreen" : "red",
                           has_skill ? "ltgreen" : "red",
                           sel_vpart_info->difficulty,
                           engine_string.c_str());
            wrefresh (w_msg);
        }
        else if (is_hand_remove) {
            werase (w_msg);
            fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                           _("Needs <color_%1$s>%2$s</color>, and level <color_%3$s>%4$d</color> skill in mechanics.%5$s"),
                           has_comps ? "ltgreen" : "red",
                           item::nname( itm ).c_str(),
                           has_skill ? "ltgreen" : "red",
                           sel_vpart_info->difficulty,
                           engine_string.c_str());
            wrefresh (w_msg);
        }
        else {
            werase (w_msg);
            fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                           _("Needs <color_%1$s>%2$s</color>, a <color_%3$s>wrench</color>, either a <color_%4$s>powered welder</color> or <color_%5$s>duct tape</color>, and level <color_%6$s>%7$d</color> skill in mechanics.%8$s"),
                           has_comps ? "ltgreen" : "red",
                           item::nname( itm ).c_str(),
                           has_wrench ? "ltgreen" : "red",
                           (has_welder && has_goggles) ? "ltgreen" : "red",
                           has_duct_tape ? "ltgreen" : "red",
                           has_skill ? "ltgreen" : "red",
                           sel_vpart_info->difficulty,
                           engine_string.c_str());
            wrefresh (w_msg);
        }
        if (has_comps && (has_tools || (is_wood && (has_hammer || has_nailgun) && has_nails) || (is_wrenchable && has_wrench) || (is_hand_remove)) && has_skill && has_skill2) {
            return true;
        }
    }
    return false;
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

    std::array<std::string,7> tab_list = { { pgettext("Vehicle Parts|","All"),
                                             pgettext("Vehicle Parts|","Cargo"),
                                             pgettext("Vehicle Parts|","Light"),
                                             pgettext("Vehicle Parts|","Util"),
                                             pgettext("Vehicle Parts|","Hull"),
                                             pgettext("Vehicle Parts|","Internal"),
                                             pgettext("Vehicle Parts|","Other") } };

    std::array<std::string,7> tab_list_short = { { pgettext("Vehicle Parts|","A"),
                                                   pgettext("Vehicle Parts|","C"),
                                                   pgettext("Vehicle Parts|","L"),
                                                   pgettext("Vehicle Parts|","U"),
                                                   pgettext("Vehicle Parts|","H"),
                                                   pgettext("Vehicle Parts|","I"),
                                                   pgettext("Vehicle Parts|","O") } };

    std::array <std::function<bool(vpart_info)>,7> tab_filters; // filter for each tab, last one
    tab_filters[0] = [&](vpart_info) { return true; }; // All
    tab_filters[1] = [&](vpart_info part) { return part.has_flag(VPFLAG_CARGO) && // Cargo
                                                   !part.has_flag("TURRET"); };
    tab_filters[2] = [&](vpart_info part) { return part.has_flag(VPFLAG_LIGHT) || // Light
                                                   part.has_flag(VPFLAG_CONE_LIGHT) ||
                                                   part.has_flag(VPFLAG_CIRCLE_LIGHT) ||
                                                   part.has_flag(VPFLAG_DOME_LIGHT) ||
                                                   part.has_flag(VPFLAG_AISLE_LIGHT) ||
                                                   part.has_flag(VPFLAG_EVENTURN) ||
                                                   part.has_flag(VPFLAG_ODDTURN); };
    tab_filters[3] = [&](vpart_info part) { return part.has_flag("TRACK") || //Util
                                                   part.has_flag(VPFLAG_FRIDGE) ||
                                                   part.has_flag("KITCHEN") ||
                                                   part.has_flag("WELDRIG") ||
                                                   part.has_flag("CRAFTRIG") ||
                                                   part.has_flag("CHEMLAB") ||
                                                   part.has_flag("FORGE") ||
                                                   part.has_flag("HORN") ||
                                                   part.has_flag(VPFLAG_RECHARGE) ||
                                                   part.has_flag("VISION") ||
                                                   part.has_flag("POWER_TRANSFER") ||
                                                   part.has_flag("FAUCET") ||
                                                   part.has_flag("STEREO") ||
                                                   part.has_flag("MUFFLER") ||
                                                   part.has_flag("REMOTE_CONTROLS") ||
                                                   part.has_flag("CURTAIN") ||
                                                   part.has_flag("SEATBELT") ||
                                                   part.has_flag("SECURITY") ||
                                                   part.has_flag("SEAT") ||
                                                   part.has_flag("BED") ||
                                                   part.has_flag("DOOR_MOTOR"); };
    tab_filters[4] = [&](vpart_info part) { return(part.has_flag(VPFLAG_OBSTACLE) || // Hull
                                                   part.has_flag("ROOF") ||
                                                   part.has_flag(VPFLAG_ARMOR)) &&
                                                   !part.has_flag("WHEEL") &&
                                                   !tab_filters[3](part); };
    tab_filters[5] = [&](vpart_info part) { return part.has_flag(VPFLAG_ENGINE) || // Internals
                                                   part.has_flag(VPFLAG_ALTERNATOR) ||
                                                   part.has_flag(VPFLAG_CONTROLS) ||
                                                   part.location == "fuel_source" ||
                                                   part.location == "on_battery_mount" ||
                                                   (part.location.empty() && part.has_flag("FUEL_TANK")); };
    tab_filters[tab_filters.size()-1] = [&](vpart_info part) { // Other: everything that's not in the other filters
        for (size_t i=1; i < tab_filters.size()-1; i++ ) {
            if( tab_filters[i](part) ) return false;
        }
        return true; };

    std::vector<vpart_info> tab_vparts = can_mount; // full list of mountable parts, to be filtered according to tab

    int pos = 0;
    size_t tab = 0;
    while (true) {
        display_list(pos, tab_vparts, 2);

        // draw tab menu
        int tab_x = 0;
        for( size_t i=0; i < tab_list.size(); i++ ){
            std::string tab_name = (tab == i) ? tab_list[i] : tab_list_short[i]; // full name for selected tab
            tab_x += (tab == i); // add a space before selected tab
            draw_subtab(w_list, tab_x, tab_name, tab == i, false);
            tab_x += ( 1 + utf8_width(tab_name.c_str()) + (tab == i) ); // one space padding and add a space after selected tab
        }
        wrefresh(w_list);

        sel_vpart_info = (tab_vparts.size() > 0) ? &(tab_vparts[pos]) : NULL; // filtered list can be empty

        display_details( sel_vpart_info );

        bool can_install = can_install_part(msg_width);

        const std::string action = main_context.handle_input();
        if (action == "INSTALL" || action == "CONFIRM"){
            if (can_install) {
                std::vector< vpart_info* > shapes = vpart_shapes[sel_vpart_info->name+sel_vpart_info->item];
                int selected_shape = -1;
                if ( shapes.size() > 1 ) { // more than one shape available, display selection
                    std::vector<uimenu_entry> shape_ui_entries;
                    for ( size_t i = 0; i < shapes.size(); i++ ) {
                        uimenu_entry entry = uimenu_entry( i, true, UIMENU_INVALID,
                                                           shapes[i]->name );
                        entry.extratxt.left = 1;
                        entry.extratxt.sym = special_symbol( shapes[i]->sym );
                        entry.extratxt.color = shapes[i]->color;
                        shape_ui_entries.push_back( entry );
                    }
                    selected_shape = uimenu( true, getbegx(w_list), list_w, getbegy(w_list),
                                             _("Choose shape:"), shape_ui_entries ).ret;
                } else { // only one shape available, default to first one
                    selected_shape = 0;
                }
                 if( 0 <= selected_shape && (size_t) selected_shape < shapes.size() ) {
                    sel_vpart_info = shapes[selected_shape];
                    sel_cmd = 'i';
                    return;
                }
            }
        } else if (action == "QUIT") {
            sel_vpart_info = NULL;
            werase (w_list);
            wrefresh (w_list);
            werase (w_msg);
            wrefresh(w_msg);
            break;
        } else if (action == "PREV_TAB" || action == "NEXT_TAB") {
            tab_vparts.clear();
            pos = 0;

            if(action == "PREV_TAB") {
                tab = ( tab < 1 ) ? tab_list.size() - 1 : tab - 1;
            } else {
                tab = ( tab < tab_list.size() - 1 ) ? tab + 1 : 0;
            }

            copy_if(can_mount.begin(), can_mount.end(), back_inserter(tab_vparts), tab_filters[tab]);
        }
        else {
            move_in_list(pos, action, tab_vparts.size(), 2);
        }
    }

    //destroy w_details
    werase(w_details);
    delwin(w_details);
    w_details = NULL;

    //restore windows that had been covered by w_details
    werase(w_stats);
    display_stats();
    display_name();
}

bool veh_interact::move_in_list(int &pos, const std::string &action, const int size, const int header) const
{
    int lines_per_page = page_size - header;
    if (action == "PREV_TAB" || action == "LEFT") {
        pos -= lines_per_page;
    } else if (action == "NEXT_TAB" || action == "RIGHT") {
        pos += lines_per_page;
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

            int xOffset = veh->parts[p].mount.x + ddx;
            int yOffset = veh->parts[p].mount.y + ddy;

            if (vertical_menu) {
                move_cursor(yOffset, -xOffset);
            } else {
                move_cursor(xOffset, yOffset);
            }
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
        sel_vpart_info = &(vehicle_part_types[sel_vehicle_part->id]);
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
                           item::nname( itm ).c_str());
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
                                 ammo_name(vehicle_part_types[ptanks[entry_num]->id].fuel_type).c_str(),
                                 vehicle_part_types[ptanks[entry_num]->id].name.c_str());
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

bool veh_interact::can_remove_part(int veh_part_index, int mech_skill, int msg_width){
    werase (w_msg);
    if (veh->can_unmount(veh_part_index)) {
        bool is_wood = veh->part_flag(veh_part_index, "NAILABLE");
        bool is_wheel = veh->part_flag(veh_part_index, "WHEEL");
        bool is_wrenchable = veh->part_flag(veh_part_index, "TOOL_WRENCH") ||
                                (is_wheel && veh->part_flag(veh_part_index, "NO_JACK"));
        bool is_hand_remove = veh->part_flag(veh_part_index, "TOOL_NONE");

        int skill_req;
        if (veh->part_flag(veh_part_index, "DIFFICULTY_REMOVE")) {
            skill_req = veh->part_info(veh_part_index).difficulty;
        } else if (is_wrenchable || is_hand_remove || is_wood) {
            skill_req = 1;
        } else {
            skill_req = 2;
        }

        bool has_skill = false;
        if (mech_skill >= skill_req) has_skill = true;

        //print necessary materials
        if(is_wood) {
            fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                           _("You need a <color_%1$s>hammer</color> and <color_%2$s>level %3$d</color> mechanics skill to remove this part."),
                           has_hammer ? "ltgreen" : "red",
                           has_skill ? "ltgreen" : "red",
                           skill_req);
        } else if (is_wrenchable) {
            fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                           _("You need a <color_%1$s>wrench</color> and <color_%2$s>level %3$d</color> mechanics skill to remove this part."),
                           has_wrench ? "ltgreen" : "red",
                           has_skill ? "ltgreen" : "red",
                           skill_req);
        } else if (is_hand_remove) {
            fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                            _("You need <color_%1$s>level %2$d</color> mechanics skill to remove this part."),
                           has_skill ? "ltgreen" : "red",
                           skill_req);
        } else if (is_wheel) {
            fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                           _("You need a <color_%1$s>wrench</color>, a <color_%2$s>jack</color> and <color_%3$s>level %4$d</color> mechanics skill to remove this part."),
                           has_wrench ? "ltgreen" : "red",
                           has_jack ? "ltgreen" : "red",
                           has_skill ? "ltgreen" : "red",
                           skill_req);
        } else {
            fold_and_print(w_msg, 0, 1, msg_width - 2, c_ltgray,
                           _("You need a <color_%1$s>wrench</color> and a <color_%2$s>hacksaw, cutting torch and goggles, or circular saw (off)</color> and <color_%3$s>level %4$d</color> mechanics skill to remove this part."),
                           has_wrench ? "ltgreen" : "red",
                           has_hacksaw ? "ltgreen" : "red",
                           has_skill ? "ltgreen" : "red",
                           skill_req);
        }
        wrefresh (w_msg);
        //check if have all necessary materials
        if (has_skill && ((is_wheel && has_wrench && has_jack) ||
                            (is_wrenchable && has_wrench) ||
                            (is_hand_remove) ||
                            (is_wood && has_hammer) ||
                            ((!is_wheel) && has_wrench && has_hacksaw) )) {
            return true;
        }
    } else {
        mvwprintz(w_msg, 0, 1, c_ltred,
                  _("You cannot remove that part while something is attached to it."));
        wrefresh (w_msg);
    }
    return false;

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
    switch (reason) {
    case LOW_MORALE:
        mvwprintz(w_msg, 0, 1, c_ltred, _("Your morale is too low to construct..."));
        wrefresh (w_msg);
        return;
    case INVALID_TARGET:
        mvwprintz(w_msg, 0, 1, c_ltred, _("No parts here."));
        wrefresh (w_msg);
        return;
    case NOT_FREE:
        mvwprintz(w_msg, 0, 1, c_ltred,
                  _("You cannot remove that part while something is attached to it."));
        wrefresh (w_msg);
        return;
    case MOVING_VEHICLE:
        fold_and_print( w_msg, 0, 1, msg_width - 2, c_ltgray,
                        _( "Better not remove something while driving." ) );
        wrefresh (w_msg);
        return;
    default:
        break; // no reason, all is well
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, _("Choose a part here to remove:"));
    wrefresh (w_mode);

    int first = 0;
    if(parts_here.size() > 1) {
        first = 1;
    }
    int pos = first;
    while (true) {
        //these variables seem to fetch the vehicle parts at specified position
        sel_vehicle_part = &veh->parts[parts_here[pos]];
        sel_vpart_info = &(vehicle_part_types[sel_vehicle_part->id]);
        //redraw list of parts
        werase (w_parts);
        veh->print_part_desc (w_parts, 0, parts_w, cpart, pos);
        wrefresh (w_parts);
        bool can_remove = can_remove_part(parts_here[pos], g->u.skillLevel("mechanics"), msg_width);
        //read input
        const std::string action = main_context.handle_input();
        if (can_remove && (action == "REMOVE" || action == "CONFIRM")) {
            sel_cmd = 'o';
            break;
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
        mvwprintz(w_msg, 0, 1, c_ltred, _("The vehicle has no fuel left to siphon."));
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
        sel_vpart_info = &(wheel_types[pos]);
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
bool veh_interact::can_currently_install(vpart_info *vpart)
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

    //Only build the shapes map once
    if (vpart_shapes.empty()) {
        for( auto &vpart_type : vehicle_part_types ) {
                vpart_shapes[vpart_type.second.name+vpart_type.second.item].push_back(&vpart_type.second);
        }
    }

    can_mount.clear();
    if (!obstruct) {
        int divider_index = 0;
        for( auto &vehicle_part_type : vehicle_part_types ) {
            if( veh->can_mount( vdx, vdy, vehicle_part_type.first ) ) {
                vpart_info *vpi = &vehicle_part_type.second;
                if ( vpi->id != vpart_shapes[vpi->name+vpi->item][0]->id )
                    continue; // only add first shape to install list
                if (can_currently_install(vpi)) {
                    can_mount.insert( can_mount.begin() + divider_index++, *vpi );
                } else {
                    can_mount.push_back( *vpi );
                }
            }
        }
    }

    //Only build the wheel list once
    if (wheel_types.empty()) {
        for( auto &vehicle_part_type : vehicle_part_types ) {
            if( vehicle_part_type.second.has_flag( "WHEEL" ) ) {
                wheel_types.push_back( vehicle_part_type.second );
            }
        }
    }

    need_repair.clear();
    parts_here.clear();
    ptanks.clear();
    has_ptank = false;
    wheel = NULL;
    if (cpart >= 0) {
        parts_here = veh->parts_at_relative(veh->parts[cpart].mount.x, veh->parts[cpart].mount.y);
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
    for( auto &structural_part : structural_parts ) {
        const int p = structural_part;
        long sym = veh->part_sym (p);
        nc_color col = veh->part_color (p);
        if (vertical_menu) {
            x =   veh->parts[p].mount.y + ddy;
            y = -(veh->parts[p].mount.x + ddx);
        } else {
            tileray tdir( 0 );
            sym = tdir.dir_symbol( sym );
            x = veh->parts[p].mount.x + ddx;
            y = veh->parts[p].mount.y + ddy;
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
    for( auto &cargo_part : cargo_parts ) {
        const int p = cargo_part;
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
        const auto iw = utf8_width(_("Most damaged: ")) + 1;
        x[6] += iw;
        w[6] -= iw;
        std::string partID = veh->parts[mostDamagedPart].id;
        vehicle_part part = veh->parts[mostDamagedPart];
        int damagepercent = 100 * part.hp / vehicle_part_types[part.id].durability;
        nc_color damagecolor = getDurabilityColor(damagepercent);
        partName = vehicle_part_types[partID].name;
        const auto hoff = fold_and_print(w_stats, y[6], x[6], w[6], damagecolor, partName);
        // If fold_and_print did write on the next line(s), shift the following entries,
        // hoff == 1 is already implied and expected - one line is consumed at least.
        for( size_t i = 7; i < sizeof(y) / sizeof(y[0]); ++i) {
            y[i] += hoff - 1;
        }
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

    bool first = true;
    int fuel_name_length = 0;
    for (int i = 0; i < num_fuel_types; ++i) {
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
                               (x[10] + 13 + fuel_name_length < stats_w),
                               !vertical_menu);

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
void veh_interact::display_list(size_t pos, std::vector<vpart_info> list, const int header)
{
    werase (w_list);
    int lines_per_page = page_size - header;
    size_t page = pos / lines_per_page;
    for (size_t i = page * lines_per_page; i < (page + 1) * lines_per_page && i < list.size(); i++) {
        int y = i - page * lines_per_page + header;
        nc_color col = can_currently_install(&list[i]) ? c_white : c_dkgray;
        mvwprintz(w_list, y, 3, pos == i ? hilite (col) : col, list[i].name.c_str());
        mvwputch (w_list, y, 1, list[i].color, special_symbol(list[i].sym));
    }
    wrefresh (w_list);
}

/**
 * Used when installing parts.
 * Opens up w_details containing info for part currently selected in w_list.
 */
void veh_interact::display_details( const vpart_info *part )
{

    if (w_details == NULL) { // create details window first if required

        // covers right part of w_name and w_stats in vertical/hybrid, lower block of w_stats in horizontal mode
        const int details_y = vertical_menu ? getbegy(w_name) : getbegy(w_stats) + stats_h - 7;
        const int details_x = vertical_menu ? getbegx(w_list) : getbegx(w_stats);

        const int details_h = vertical_menu ? 6 : 7; // 6 lines in vertical/hybrid, 7 lines in horizontal mode
        const int details_w = getbegx(w_grid) + getmaxx(w_grid) - details_x;

        if (vertical_menu) { // clear rightmost blocks of w_stats in vertical/hybrid mode to avoid overlap
            int stats_col_2 = 34;
            int stats_col_3 = 63 + ((TERMX - FULL_SCREEN_WIDTH) / 4);
            int clear_x = stats_w - details_w + 1 >= stats_col_3 ? stats_col_3 : stats_col_2;
            for( int i = 0; i < stats_h; i++) {
                mvwhline(w_stats, i, clear_x, ' ', stats_w - clear_x);
            }
        } else { // clear one line above w_details in horizontal mode to make sure it's separated from stats text
            mvwhline(w_stats, details_y - getbegy(w_stats) - 1, 0, ' ', stats_w);
        }
        wrefresh(w_stats);

        w_details = newwin(details_h, details_w, details_y, details_x);
    }
    else {
        werase(w_details);
   }

    wborder(w_details, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);

    if ( part == NULL ) {
        wrefresh(w_details);
        return;
    }
    int details_w = getmaxx(w_details);
    int column_width = details_w / 2; // displays data in two columns
    int col_1 = vertical_menu ? 2 : 1;
    int col_2 = col_1 + column_width;
    int line = vertical_menu ? 0 : 0;
    bool small_mode = column_width < 20 ? true : false;

    // line 0: part name
    fold_and_print(w_details, line, col_1, details_w, c_ltgreen,
                   part->name);

    // line 1: (column 1) durability   (column 2) damage mod
    fold_and_print(w_details, line+1, col_1, column_width, c_white,
                   "%s: <color_ltgray>%d</color>",
                   small_mode ? _("Dur") : _("Durability"),
                   part->durability);
    fold_and_print(w_details, line+1, col_2, column_width, c_white,
                   "%s: <color_ltgray>%d%%</color>",
                   small_mode ? _("Dmg") : _("Damage"),
                   part->dmg_mod);

    // line 2: (column 1) weight   (column 2) folded volume (if applicable)
    fold_and_print(w_details, line+2, col_1, column_width, c_white,
                   "%s: <color_ltgray>%.1f%s</color>",
                   small_mode ? _("Wgt") : _("Weight"),
                   g->u.convert_weight(item::find_type( part->item )->weight),
                   OPTIONS["USE_METRIC_WEIGHTS"].getValue() == "lbs" ? "lb" : "kg");
    if ( part->folded_volume != 0 ) {
        fold_and_print(w_details, line+2, col_2, column_width, c_white,
                       "%s: <color_ltgray>%d</color>",
                       small_mode ? _("FoldVol") : _("Folded Volume"),
                       part->folded_volume);
    }

    // line 3: (column 1) par1,size,bonus,wheel_width (as applicable)    (column 2) epower (if applicable)
    if ( part->size > 0 ) {

        std::string label;
        if ( part->has_flag(VPFLAG_CARGO) || part->has_flag(VPFLAG_FUEL_TANK) ) {
            label = small_mode ? _("Cap") : _("Capacity");
        } else if ( part->has_flag(VPFLAG_WHEEL) ){
            label = small_mode ? _("Size") : _("Wheel Size");
        } else if ( part->has_flag(VPFLAG_SEATBELT) || part->has_flag("MUFFLER") ) {
            label = small_mode ? _("Str") : _("Strength");
        } else if ( part->has_flag("HORN") ) {
            label = _("Noise");
        } else if ( part->has_flag(VPFLAG_EXTENDS_VISION) ) {
            label = _("Range");
        } else if ( part->has_flag(VPFLAG_LIGHT) || part->has_flag(VPFLAG_CONE_LIGHT) ||
                    part->has_flag(VPFLAG_CIRCLE_LIGHT) || part->has_flag(VPFLAG_DOME_LIGHT) ||
                    part->has_flag(VPFLAG_AISLE_LIGHT) || part->has_flag(VPFLAG_EVENTURN) ||
                    part->has_flag(VPFLAG_ODDTURN)) {
            label = _("Light");
        } else {
            label = small_mode ? _("Cap") : _("Capacity");
        }

        fold_and_print(w_details, line+3, col_1, column_width, c_white,
                       (label + ": <color_ltgray>%d</color>").c_str(),
                       part->size);
    }
    if ( part->epower != 0 ) {
        fold_and_print(w_details, line+3, col_2, column_width, c_white,
                       "%s: %c<color_ltgray>%d</color>",
                       small_mode ? _("Bat") : _("Battery"),
                       part->epower < 0 ? '-' : '+',
                       abs(part->epower));
    }

    // line 4 [horizontal]: fuel_type (if applicable)
    // line 4 [vertical/hybrid]: (column 1) fuel_type (if applicable)    (column 2) power (if applicable)
    // line 5 [horizontal]: power (if applicable)
    if ( part->fuel_type != "NULL" ) {
        fold_and_print(w_details, line+4, col_1, ( vertical_menu ? column_width : details_w ), c_white,
                       _("Charge: <color_ltgray>%s</color>"),
                       part->fuel_type.c_str());
    }
    if ( part->power != 0 ) {
        fold_and_print(w_details, ( vertical_menu ? line+4 : line+5 ), ( vertical_menu ? col_2 : col_1 ),
                       ( vertical_menu ? column_width : details_w ), c_white,
                       _("Power: <color_ltgray>%d</color>"),
                       part->power);
    }

    // line 5 [vertical/hybrid] 6 [horizontal]: flags
    std::vector<std::string> flags = { { "OPAQUE", "OPENABLE", "BOARDABLE" } };
    std::vector<std::string> flag_labels = { { _("opaque"), _("openable"), _("boardable") } };
    std::string label;
    for ( size_t i = 0; i < flags.size(); i++ ) {
        if ( part->has_flag(flags[i]) ) {
            label += ( label.empty() ? "" : " " ) + flag_labels[i];
        }
    }
    fold_and_print(w_details, ( vertical_menu ? line+5 : line+6 ), col_1, details_w, c_yellow, label);

    wrefresh(w_details);
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
    map_inv.form_from_map( point(g->u.posx(), g->u.posy()), PICKUP_RANGE );

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
        for( const auto &candidate : candidates ) {
            if( candidate ) {
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
        item_used = g->m.use_amount( point(g->u.posx(), g->u.posy()), PICKUP_RANGE, itid, 1 );
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
    int welder_charges = charges_per_use( "welder" );
    int welder_oxy_charges = charges_per_use( "oxy_torch" );
    int welder_crude_charges = charges_per_use( "welder_crude" );
    const inventory &crafting_inv = g->u.crafting_inventory();
    const bool has_goggles = crafting_inv.has_tools("goggles_welding", 1) ||
                             g->u.has_bionic("bio_sunglasses") ||
                             g->u.is_wearing("goggles_welding") || g->u.is_wearing("rm13_armor_on");
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

    bool is_wheel = vehicle_part_types[part_id].has_flag("WHEEL");
    bool is_wood = vehicle_part_types[part_id].has_flag("NAILABLE");
    bool is_wrenchable = vehicle_part_types[part_id].has_flag("TOOL_WRENCH");
    bool is_hand_remove = vehicle_part_types[part_id].has_flag("TOOL_NONE");

    // cmd = Install Repair reFill remOve Siphon Drainwater Changetire reName relAbel
    switch (cmd) {
    case 'i':
        if(is_wood) {
            tools.push_back(tool_comp("nail", NAILS_USED));
            g->u.consume_tools(tools);
        }
        // Only parts that use charges
        else if (!is_wrenchable && !is_hand_remove){
            if (has_goggles) {
                // Need welding goggles to use any of these tools,
                // without the goggles one _must_ use the duct tape
                tools.push_back(tool_comp("welder", welder_charges));
                tools.push_back(tool_comp("oxy_torch", welder_oxy_charges));
                tools.push_back(tool_comp("welder_crude", welder_crude_charges));
                tools.push_back(tool_comp("toolset", welder_crude_charges));
            }
            tools.push_back(tool_comp("duct_tape", DUCT_TAPE_USED));
            tools.push_back(tool_comp("toolbox", DUCT_TAPE_USED));
            g->u.consume_tools(tools);
        }

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
            g->u.view_offset_x = veh->global_x() + gx - g->u.posx();
            g->u.view_offset_y = veh->global_y() + gy - g->u.posy();
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
        // easy parts don't train
        if (!is_wrenchable && !is_hand_remove) {
            g->u.practice( "mechanics", vehicle_part_types[part_id].difficulty * 5 + (is_wood ? 10 : 20) );
        }
        break;
    case 'r':
        veh->last_repair_turn = calendar::turn;
        if (veh->parts[vehicle_part].hp <= 0) {
            veh->break_part_into_pieces(vehicle_part, g->u.posx(), g->u.posy());
            used_item = consume_vpart_item (veh->parts[vehicle_part].id);
            veh->parts[vehicle_part].bigness = used_item.bigness;
            dd = 0;
            veh->insides_dirty = true;
        } else {
            dmg = 1.1 - double(veh->parts[vehicle_part].hp) / veh->part_info(vehicle_part).durability;
        }
        if (has_goggles) {
            // Need welding goggles to use any of these tools,
            // without the goggles one _must_ use the duct tape
            tools.push_back(tool_comp("welder", int(welder_charges * dmg)));
            tools.push_back(tool_comp("oxy_torch", int(welder_oxy_charges * dmg)));
            tools.push_back(tool_comp("welder_crude", int(welder_crude_charges * dmg)));
            tools.push_back(tool_comp("toolset", int(welder_crude_charges * dmg)));
        }
        tools.push_back(tool_comp("duct_tape", int(DUCT_TAPE_USED * dmg)));
        tools.push_back(tool_comp("toolbox", int(DUCT_TAPE_USED * dmg)));
        g->u.consume_tools(tools);
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
        // Only parts that use charges
        if (!is_wrenchable && !is_hand_remove && !is_wheel){
            if( !crafting_inv.has_items_with_quality( "SAW_M_FINE", 1, 1 ) && (is_wood && !crafting_inv.has_items_with_quality("HAMMER", 1, 1))) {
                tools.push_back(tool_comp("circsaw_off", 20));
                tools.push_back(tool_comp("oxy_torch", 10));
                g->u.consume_tools(tools);
            }
        }
        // Nails survive if pulled out with pliers or a claw hammer. TODO: implement PLY tool quality and random chance based on skill/quality
        /*if(is_wood && crafting_inv.has_items_with_quality("HAMMER", 1, 1)) {
            item return_nails("nail");
            return_nails.charges = 10;
            g->m.add_item_or_charges( g->u.posx, g->u.posy, return_nails );
        }*/ //causes runtime errors. not a critical feature; implement with PLY quality.
        // Dump contents of part at player's feet, if any.
        for( auto &elem : veh->get_items(vehicle_part) ) {
            g->m.add_item_or_charges( g->u.posx(), g->u.posy(), elem );
        }
        while( !veh->get_items(vehicle_part).empty() ) {
            veh->get_items(vehicle_part).erase( veh->get_items(vehicle_part).begin() );
        }

        // Power cables must remove parts from the target vehicle, too.
        if (veh->part_flag(vehicle_part, "POWER_TRANSFER")) {
            veh->remove_remote_part(vehicle_part);
        }

        broken = veh->parts[vehicle_part].hp <= 0;
        if (!broken) {
            used_item = veh->parts[vehicle_part].properties_to_item();
            g->m.add_item_or_charges(g->u.posx(), g->u.posy(), used_item);
            // simple tasks won't train mechanics
            if(type != SEL_JACK && !is_wrenchable && !is_hand_remove) {
                g->u.practice ("mechanics", is_wood ? 15 : 30);
            }
        } else {
            veh->break_part_into_pieces(vehicle_part, g->u.posx(), g->u.posy());
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
        for (int x = g->u.posx() - 1; x < g->u.posx() + 2; x++) {
            for (int y = g->u.posy() - 1; y < g->u.posy() + 2; y++) {
                fillv = g->m.veh_at(x, y);
                if ( fillv != NULL &&
                     fillv != veh &&
                     foundv.find( point(fillv->posx, fillv->posy) ) == foundv.end() &&
                     (fillv->fuel_capacity("gasoline") > 0 || fillv->fuel_capacity("diesel") > 0)) {
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
    { // Weird indention to avoid moving this whole block over 4 spaces.
        int menu_choice = -1;
        if( veh->fuel_left("gasoline") > 0 && veh->fuel_left("diesel") > 0 ) {
            uimenu smenu;
            smenu.text = _("Siphon what?");
            smenu.addentry(_("Gasoline"));
            smenu.addentry(_("Diesel"));
            smenu.addentry(_("Never mind"));
            smenu.query();
            menu_choice = smenu.ret;
        }
        if( fillv != NULL ) {
            int want = 0;
            int got = 0;
            std::string choice = "none";

            if( menu_choice != -1 ) {

                if ( menu_choice == 0 ) {
                    choice = "gasoline";
                    want = fillv->fuel_capacity("gasoline") - fillv->fuel_left("gasoline");
                    got = veh->drain("gasoline", want);
                    fillv->refill("gasoline", got);
                } else if( menu_choice == 1 ) {
                    choice = "diesel";
                    want = fillv->fuel_capacity("diesel") - fillv->fuel_left("diesel");
                    got = veh->drain("diesel", want);
                    fillv->refill("diesel", got);
                } else {
                    choice = "none";
                    break;
                }
                g->u.moves -= 200;
            } else if( veh->fuel_capacity("diesel") > 0 ) {
                want = fillv->fuel_capacity("diesel") - fillv->fuel_left("diesel");
                got = veh->drain("diesel", want);
                fillv->refill("diesel",got);
                g->u.moves -= 200;
            } else if( veh->fuel_capacity("gasoline") > 0 ) {
                want = fillv->fuel_capacity("gasoline") - fillv->fuel_left("gasoline");
                got = veh->drain("gasoline", want);
                fillv->refill("gasoline",got);
                g->u.moves -= 200;
            } else {
                add_msg(_("That vehicle has no fuel to siphon."));
                break;
            }
            // Common message for all cases, no fuel case breaks and avoids it..
            if( got < want ) {
                add_msg(_("Siphoned %d units of %s from the %s into the %s, draining the tank."),
                        got, choice.c_str(), veh->name.c_str(), fillv->name.c_str() );
            } else {
                add_msg(_("Siphoned %d units of %s from the %s into the %s, receiving tank is full."),
                        got, choice.c_str(), veh->name.c_str(), fillv->name.c_str() );
            }
        } else {
            if( menu_choice != -1 ) {
                if( menu_choice == 0 ) {
                    g->u.siphon( veh, "gasoline" );
                } else if( menu_choice == 1 ) {
                    g->u.siphon( veh, "diesel" );
                } else {
                    break;
                }
            } else if( veh->fuel_left("diesel") > 0 ) {
                g->u.siphon( veh, "diesel" );
            } else if( veh->fuel_left("gasoline") > 0 ) {
                g->u.siphon( veh, "gasoline" );
            } else {
                add_msg(_("That vehicle has no fuel to siphon."));
                break;
            }
        }
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
                g->m.add_item_or_charges( g->u.posx(), g->u.posy(), removed_wheel );
            }
        }
        break;
    case 'd':
        g->u.siphon( veh, "water" );
        break;
    }
    g->u.invalidate_crafting_inventory();
}
