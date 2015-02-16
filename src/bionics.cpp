#include "player.h"
#include "game.h"
#include "rng.h"
#include "input.h"
#include "item.h"
#include "bionics.h"
#include "line.h"
#include "json.h"
#include "messages.h"
#include "overmapbuffer.h"
#include "sounds.h"

#include <math.h>    //sqrt
#include <algorithm> //std::min
#include <sstream>

#define BATTERY_AMOUNT 100 // How much batteries increase your power

std::map<bionic_id, bionic_data *> bionics;
std::vector<bionic_id> faulty_bionics;
std::vector<bionic_id> power_source_bionics;
std::vector<bionic_id> unpowered_bionics;

void bionics_install_failure(player *u, int difficulty, int success);

bionic_data::bionic_data(std::string nname, bool ps, bool tog, int pac, int pad, int pot,
                          int ct, std::string desc, bool fault) : description(desc)
{
    name = nname;
    power_source = ps;
    activated = tog || pac || ct;
    toggled = tog;
    power_activate = pac;
    power_deactivate = pad;
    power_over_time = pot;
    charge_time = ct;
    faulty = fault;
}

bionic_id game::random_good_bionic() const
{
    std::map<std::string, bionic_data *>::const_iterator random_bionic;
    do {
        random_bionic = bionics.begin();
        std::advance(random_bionic, rng(0, bionics.size() - 1));
    } while (random_bionic->second->faulty);
    return random_bionic->first;
}

void show_bionics_titlebar(WINDOW *window, player *p, std::string menu_mode)
{
    werase(window);

    std::string caption = _("BIONICS -");
    int cap_offset = utf8_width(caption.c_str()) + 1;
    mvwprintz(window, 0,  0, c_blue, "%s", caption.c_str());

    std::stringstream pwr;
    pwr << string_format(_("Power: %i/%i"), int(p->power_level), int(p->max_power_level));
    int pwr_length = utf8_width(pwr.str().c_str()) + 1;
    mvwprintz(window, 0, getmaxx(window) - pwr_length, c_white, "%s", pwr.str().c_str());

    std::string desc;
    int desc_length = getmaxx(window) - cap_offset - pwr_length;

    if(menu_mode == "reassigning") {
        desc = _("Reassigning.\nSelect a bionic to reassign or press SPACE to cancel.");
    } else if(menu_mode == "activating") {
        desc = _("<color_green>Activating</color>  <color_yellow>!</color> to examine, <color_yellow>-</color> to remove, <color_yellow>=</color> to reassign.");
    } else if(menu_mode == "removing") {
        desc = _("<color_red>Removing</color>  <color_yellow>!</color> to activate, <color_yellow>-</color> to remove, <color_yellow>=</color> to reassign.");
    } else if(menu_mode == "examining") {
        desc = _("<color_ltblue>Examining</color>  <color_yellow>!</color> to activate, <color_yellow>-</color> to remove, <color_yellow>=</color> to reassign.");
    }
    fold_and_print(window, 0, cap_offset, desc_length, c_white, desc);

    wrefresh(window);
}

void player::power_bionics()
{
    std::vector <bionic *> passive;
    std::vector <bionic *> active;
    for( auto &elem : my_bionics ) {
        if( !bionics[elem.id]->activated ) {
            passive.push_back( &elem );
        } else {
            active.push_back( &elem );
        }
    }

    // maximal number of rows in both columns
    const int bionic_count = std::max(passive.size(), active.size());

    int TITLE_HEIGHT = 2;
    int DESCRIPTION_HEIGHT = 5;

    // Main window
    /** Total required height is:
    * top frame line:                                         + 1
    * height of title window:                                 + TITLE_HEIGHT
    * line after the title:                                   + 1
    * line with active/passive bionic captions:               + 1
    * height of the biggest list of active/passive bionics:   + bionic_count
    * line before bionic description:                         + 1
    * height of description window:                           + DESCRIPTION_HEIGHT
    * bottom frame line:                                      + 1
    * TOTAL: TITLE_HEIGHT + bionic_count + DESCRIPTION_HEIGHT + 5
    */
    int HEIGHT = std::min(TERMY, std::max(FULL_SCREEN_HEIGHT,
                                          TITLE_HEIGHT + bionic_count + DESCRIPTION_HEIGHT + 5));
    int WIDTH = FULL_SCREEN_WIDTH + (TERMX - FULL_SCREEN_WIDTH) / 2;
    int START_X = (TERMX - WIDTH) / 2;
    int START_Y = (TERMY - HEIGHT) / 2;
    WINDOW *wBio = newwin(HEIGHT, WIDTH, START_Y, START_X);
    WINDOW_PTR wBioptr( wBio );

    // Description window @ the bottom of the bio window
    int DESCRIPTION_START_Y = START_Y + HEIGHT - DESCRIPTION_HEIGHT - 1;
    int DESCRIPTION_LINE_Y = DESCRIPTION_START_Y - START_Y - 1;
    WINDOW *w_description = newwin(DESCRIPTION_HEIGHT, WIDTH - 2,
                                   DESCRIPTION_START_Y, START_X + 1);
    WINDOW_PTR w_descriptionptr( w_description );

    // Title window
    int TITLE_START_Y = START_Y + 1;
    int HEADER_LINE_Y = TITLE_HEIGHT + 1; // + lines with text in titlebar, local
    WINDOW *w_title = newwin(TITLE_HEIGHT, WIDTH - 2, TITLE_START_Y, START_X + 1);
    WINDOW_PTR w_titleptr( w_title );

    int scroll_position = 0;
    int second_column = 32 + (TERMX - FULL_SCREEN_WIDTH) /
                        4; // X-coordinate of the list of active bionics

    input_context ctxt("BIONICS");
    ctxt.register_updown();
    ctxt.register_action("ANY_INPUT");
    ctxt.register_action("TOOGLE_EXAMINE");
    ctxt.register_action("REASSIGN");
    ctxt.register_action("REMOVE");
    ctxt.register_action("HELP_KEYBINDINGS");

    bool redraw = true;
    std::string menu_mode = "activating";

    while(true) {
        // offset for display: bionic with index i is drawn at y=list_start_y+i
        // drawing the bionics starts with bionic[scroll_position]
        const int list_start_y = HEADER_LINE_Y + 2 - scroll_position;
        int max_scroll_position = HEADER_LINE_Y + 2 + bionic_count -
                                  ((menu_mode == "examining") ? DESCRIPTION_LINE_Y : (HEIGHT - 1));
        if(redraw) {
            redraw = false;

            werase(wBio);
            draw_border(wBio);
            // Draw line under title
            mvwhline(wBio, HEADER_LINE_Y, 1, LINE_OXOX, WIDTH - 2);
            // Draw symbols to connect additional lines to border
            mvwputch(wBio, HEADER_LINE_Y, 0, BORDER_COLOR, LINE_XXXO); // |-
            mvwputch(wBio, HEADER_LINE_Y, WIDTH - 1, BORDER_COLOR, LINE_XOXX); // -|

            // Captions
            mvwprintz(wBio, HEADER_LINE_Y + 1, 2, c_ltblue, _("Passive:"));
            mvwprintz(wBio, HEADER_LINE_Y + 1, second_column, c_ltblue, _("Active:"));

            draw_exam_window(wBio, DESCRIPTION_LINE_Y, menu_mode == "examining");
            nc_color type;
            if (passive.empty()) {
                mvwprintz(wBio, list_start_y, 2, c_ltgray, _("None"));
            } else {
                for (size_t i = scroll_position; i < passive.size(); i++) {
                    if (list_start_y + static_cast<int>(i) ==
                        (menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1)) {
                        break;
                    }
                    if (bionics[passive[i]->id]->power_source) {
                        type = c_ltcyan;
                    } else {
                        type = c_cyan;
                    }
                    mvwprintz(wBio, list_start_y + i, 2, type, "%c %s", passive[i]->invlet,
                              bionics[passive[i]->id]->name.c_str());
                }
            }

            if (active.empty()) {
                mvwprintz(wBio, list_start_y, second_column, c_ltgray, _("None"));
            } else {
                for (size_t i = scroll_position; i < active.size(); i++) {
                    if (list_start_y + static_cast<int>(i) ==
                        (menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1)) {
                        break;
                    }
                    if (active[i]->powered && !bionics[active[i]->id]->power_source) {
                        type = c_red;
                    } else if (bionics[active[i]->id]->power_source && !active[i]->powered) {
                        type = c_ltcyan;
                    } else if (bionics[active[i]->id]->power_source && active[i]->powered) {
                        type = c_ltgreen;
                    } else {
                        type = c_ltred;
                    }
                    mvwputch(wBio, list_start_y + i, second_column, type, active[i]->invlet);
                    std::stringstream power_desc;
                    power_desc << bionics[active[i]->id]->name;
                    if (bionics[active[i]->id]->power_over_time > 0 && bionics[active[i]->id]->charge_time > 0) {
                        power_desc << string_format(_(", %d PU / %d turns"),
                                        bionics[active[i]->id]->power_over_time,
                                        bionics[active[i]->id]->charge_time);
                    }
                    if (bionics[active[i]->id]->power_activate > 0) {
                        power_desc << string_format(_(", %d PU act"),
                                        bionics[active[i]->id]->power_activate);
                    }
                    if (bionics[active[i]->id]->power_deactivate > 0) {
                        power_desc << string_format(_(", %d PU deact"),
                                        bionics[active[i]->id]->power_deactivate);
                    }
                    if (active[i]->powered) {
                        power_desc << _(", ON");
                    }
                    std::string tmp = power_desc.str();
                    mvwprintz(wBio, list_start_y + i, second_column + 2, type, tmp.c_str());
                }
            }

            // Scrollbar
            if(scroll_position > 0) {
                mvwputch(wBio, HEADER_LINE_Y + 2, 0, c_ltgreen, '^');
            }
            if(scroll_position < max_scroll_position && max_scroll_position > 0) {
                mvwputch(wBio, (menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1) - 1,
                         0, c_ltgreen, 'v');
            }
        }
        wrefresh(wBio);
        show_bionics_titlebar(w_title, this, menu_mode);
        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        bionic *tmp = NULL;
        if (menu_mode == "reassigning") {
            menu_mode = "activating";
            tmp = bionic_by_invlet(ch);
            if(tmp == 0) {
                // Selected an non-existing bionic (or escape, or ...)
                continue;
            }
            redraw = true;
            const char newch = popup_getkey(_("%s; enter new letter."),
                                            bionics[tmp->id]->name.c_str());
            wrefresh(wBio);
            if(newch == ch || newch == ' ' || newch == KEY_ESCAPE) {
                continue;
            }
            bionic *otmp = bionic_by_invlet(newch);
            // if there is already a bionic with the new invlet, the invlet
            // is considered valid.
            if(otmp == 0 && inv_chars.find(newch) == std::string::npos) {
                // TODO separate list of letters for bionics
                popup(_("%c is not a valid inventory letter."), newch);
                continue;
            }
            if(otmp != 0) {
                std::swap(tmp->invlet, otmp->invlet);
            } else {
                tmp->invlet = newch;
            }
            // TODO: show a message like when reassigning a key to an item?
        } else if (action == "DOWN") {
            if(scroll_position < max_scroll_position) {
                scroll_position++;
                redraw = true;
            }
        } else if (action == "UP") {
            if(scroll_position > 0) {
                scroll_position--;
                redraw = true;
            }
        } else if (action == "REASSIGN") {
            menu_mode = "reassigning";
        } else if (action == "TOOGLE_EXAMINE") { // switches between activation and examination
            menu_mode = menu_mode == "activating" ? "examining" : "activating";
            werase(w_description);
            draw_exam_window(wBio, DESCRIPTION_LINE_Y, false);
            redraw = true;
        } else if (action == "REMOVE") {
            menu_mode = "removing";
            redraw = true;
        } else if (action == "HELP_KEYBINDINGS") {
            redraw = true;
        } else {
            tmp = bionic_by_invlet(ch);
            if(tmp == 0) {
                // entered a key that is not mapped to any bionic,
                // -> leave screen
                break;
            }
            const std::string &bio_id = tmp->id;
            const bionic_data &bio_data = *bionics[bio_id];
            if (menu_mode == "removing") {
                uninstall_bionic(bio_id);
                break;
            }
            if (menu_mode == "activating") {
                if (bio_data.activated) {
                    int b = tmp - &my_bionics[0];
                    if (tmp->powered) {
                        deactivate_bionic(b);
                    } else {
                        // this will clear the bionics menu for targeting purposes
                        g->draw();
                        redraw = !activate_bionic(b);
                    }
                    if (redraw) {
                        // To update message on the sidebar
                        g->refresh_all();
                        continue;
                    } else {
                        // Action done, leave screen
                        return;
                    }
                } else {
                    popup(_("\
You can not activate %s!  To read a description of \
%s, press '!', then '%c'."), bio_data.name.c_str(), bio_data.name.c_str(), tmp->invlet);
                    redraw = true;
                }
            }
            if (menu_mode == "examining") { // Describing bionics, not activating them!
                draw_exam_window(wBio, DESCRIPTION_LINE_Y, true);
                // Clear the lines first
                werase(w_description);
                fold_and_print(w_description, 0, 0, WIDTH - 2, c_ltblue, bio_data.description);
                wrefresh(w_description);
            }
        }
    }
}

void draw_exam_window(WINDOW *win, int border_line, bool examination)
{
    int width = getmaxx(win);
    if (examination) {
        for (int i = 1; i < width - 1; i++) {
            mvwputch(win, border_line, i, BORDER_COLOR, LINE_OXOX); // Draw line above description
        }
        mvwputch(win, border_line, 0, BORDER_COLOR, LINE_XXXO); // |-
        mvwputch(win, border_line, width - 1, BORDER_COLOR, LINE_XOXX); // -|
    } else {
        for (int i = 1; i < width - 1; i++) {
            mvwprintz(win, border_line, i, c_black, " "); // Erase line
        }
        mvwputch(win, border_line, 0, BORDER_COLOR, LINE_XOXO); // |
        mvwputch(win, border_line, width, BORDER_COLOR, LINE_XOXO); // |
    }
}

// Why put this in a Big Switch?  Why not let bionics have pointers to
// functions, much like monsters and items?
//
// Well, because like diseases, which are also in a Big Switch, bionics don't
// share functions....
bool player::activate_bionic(int b, bool eff_only)
{
    bionic &bio = my_bionics[b];

    // Special compatibility code for people who updated saves with their claws out
    if ((weapon.type->id == "bio_claws_weapon" && bio.id == "bio_claws_weapon") ||
        (weapon.type->id == "bio_blade_weapon" && bio.id == "bio_blade_weapon")) {
        return deactivate_bionic(b);
    }

    // eff_only means only do the effect without messing with stats or displaying messages
    if (!eff_only) {
        if (bio.powered) {
            // It's already on!
            return false;
        }
        if (power_level < bionics[bio.id]->power_activate) {
            add_msg(m_info, _("You don't have the power to activate your %s."), bionics[bio.id]->name.c_str());
            return false;
        }

        //We can actually activate now, do activation-y things
        power_level -= bionics[bio.id]->power_activate;
        if (bionics[bio.id]->toggled || bionics[bio.id]->charge_time > 0) {
            bio.powered = true;
        }
        if (bionics[bio.id]->charge_time > 0) {
            bio.charge = bionics[bio.id]->charge_time;
        }
        add_msg(m_info, _("You activate your %s."), bionics[bio.id]->name.c_str());
    }

    std::vector<point> traj;
    std::vector<std::string> good;
    std::vector<std::string> bad;
    int dirx, diry;
    item tmp_item;
    w_point weatherPoint = g->weatherGen.get_weather(pos(), calendar::turn);

    // On activation effects go here
    if(bio.id == "bio_painkiller") {
        pkill += 6;
        pain -= 2;
        if (pkill > pain) {
            pkill = pain;
        }
    } else if (bio.id == "bio_cqb") {
        pick_style();
    } else if (bio.id == "bio_nanobots") {
        remove_effect("bleed");
        healall(4);
    } else if (bio.id == "bio_resonator") {
        //~Sound of a bionic sonic-resonator shaking the area
        sounds::sound(posx(), posy(), 30, _("VRRRRMP!"));
        for (int i = posx() - 1; i <= posx() + 1; i++) {
            for (int j = posy() - 1; j <= posy() + 1; j++) {
                g->m.bash( i, j, 110 );
                g->m.bash( i, j, 110 ); // Multibash effect, so that doors &c will fall
                g->m.bash( i, j, 110 );
            }
        }
    } else if (bio.id == "bio_time_freeze") {
        moves += power_level;
        power_level = 0;
        add_msg(m_good, _("Your speed suddenly increases!"));
        if (one_in(3)) {
            add_msg(m_bad, _("Your muscles tear with the strain."));
            apply_damage( nullptr, bp_arm_l, rng( 5, 10 ) );
            apply_damage( nullptr, bp_arm_r, rng( 5, 10 ) );
            apply_damage( nullptr, bp_leg_l, rng( 7, 12 ) );
            apply_damage( nullptr, bp_leg_r, rng( 7, 12 ) );
            apply_damage( nullptr, bp_torso, rng( 5, 15 ) );
        }
        if (one_in(5)) {
            add_effect("teleglow", rng(50, 400));
        }
    } else if (bio.id == "bio_teleport") {
        g->teleport();
        add_effect("teleglow", 300);
    // TODO: More stuff here (and bio_blood_filter)
    } else if(bio.id == "bio_blood_anal") {
        WINDOW *w = newwin(20, 40, 3 + ((TERMY > 25) ? (TERMY - 25) / 2 : 0),
                           10 + ((TERMX > 80) ? (TERMX - 80) / 2 : 0));
        draw_border(w);
        if (has_effect("fungus")) {
            bad.push_back(_("Fungal Parasite"));
        }
        if (has_effect("dermatik")) {
            bad.push_back(_("Insect Parasite"));
        }
        if (has_effect("stung")) {
            bad.push_back(_("Stung"));
        }
        if (has_effect("poison")) {
            bad.push_back(_("Poison"));
        }
        if (radiation > 0) {
            bad.push_back(_("Irradiated"));
        }
        if (has_effect("pkill1")) {
            good.push_back(_("Minor Painkiller"));
        }
        if (has_effect("pkill2")) {
            good.push_back(_("Moderate Painkiller"));
        }
        if (has_effect("pkill3")) {
            good.push_back(_("Heavy Painkiller"));
        }
        if (has_effect("pkill_l")) {
            good.push_back(_("Slow-Release Painkiller"));
        }
        if (has_effect("drunk")) {
            good.push_back(_("Alcohol"));
        }
        if (has_effect("cig")) {
            good.push_back(_("Nicotine"));
        }
        if (has_effect("meth")) {
            good.push_back(_("Methamphetamines"));
        }
        if (has_effect("high")) {
            good.push_back(_("Intoxicant: Other"));
        }
        if (has_effect("weed_high")) {
            good.push_back(_("THC Intoxication"));
        }
        if (has_effect("hallu") || has_effect("visuals")) {
            bad.push_back(_("Hallucinations"));
        }
        if (has_effect("iodine")) {
            good.push_back(_("Iodine"));
        }
        if (has_effect("datura")) {
            good.push_back(_("Anticholinergic Tropane Alkaloids"));
        }
        if (has_effect("took_xanax")) {
            good.push_back(_("Xanax"));
        }
        if (has_effect("took_prozac")) {
            good.push_back(_("Prozac"));
        }
        if (has_effect("took_flumed")) {
            good.push_back(_("Antihistamines"));
        }
        if (has_effect("adrenaline")) {
            good.push_back(_("Adrenaline Spike"));
        }
        if (has_effect("adrenaline_mycus")) {
            good.push_back(_("Mycal Spike"));
        }
        if (has_effect("tapeworm")) {  // This little guy is immune to the blood filter though, as he lives in your bowels.
            good.push_back(_("Intestinal Parasite"));
        }
        if (has_effect("bloodworms")) {
            good.push_back(_("Hemolytic Parasites"));
        }
        if (has_effect("brainworm")) {  // This little guy is immune to the blood filter too, as he lives in your brain.
            good.push_back(_("Intracranial Parasite"));
        }
        if (has_effect("paincysts")) {  // These little guys are immune to the blood filter too, as they live in your muscles.
            good.push_back(_("Intramuscular Parasites"));
        }
        if (has_effect("tetanus")) {  // Tetanus infection.
            good.push_back(_("Clostridium Tetani Infection"));
        }
        if (good.empty() && bad.empty()) {
            mvwprintz(w, 1, 1, c_white, _("No effects."));
        } else {
            for (unsigned line = 1; line < 39 && line <= good.size() + bad.size(); line++) {
                if (line <= bad.size()) {
                    mvwprintz(w, line, 1, c_red, "%s", bad[line - 1].c_str());
                } else {
                    mvwprintz(w, line, 1, c_green, "%s", good[line - 1 - bad.size()].c_str());
                }
            }
        }
        wrefresh(w);
        refresh();
        getch();
        delwin(w);
    } else if(bio.id == "bio_blood_filter") {
        remove_effect("fungus");
        remove_effect("dermatik");
        remove_effect("bloodworms");
        remove_effect("tetanus");
        remove_effect("poison");
        remove_effect("stung");
        remove_effect("pkill1");
        remove_effect("pkill2");
        remove_effect("pkill3");
        remove_effect("pkill_l");
        remove_effect("drunk");
        remove_effect("cig");
        remove_effect("high");
        remove_effect("hallu");
        remove_effect("visuals");
        remove_effect("iodine");
        remove_effect("datura");
        remove_effect("took_xanax");
        remove_effect("took_prozac");
        remove_effect("took_flumed");
        remove_effect("adrenaline");
        remove_effect("meth");
        pkill = 0;
        stim = 0;
    } else if(bio.id == "bio_evap") {
        item water = item("water_clean", 0);
        int humidity = weatherPoint.humidity;
        int water_charges = (humidity * 3.0) / 100.0 + 0.5;
        // At 50% relative humidity or more, the player will draw 2 units of water
        // At 16% relative humidity or less, the player will draw 0 units of water
        water.charges = water_charges;
        if (water_charges == 0) {
            add_msg_if_player(m_bad, _("There was not enough moisture in the air from which to draw water!"));
        } else if (g->handle_liquid(water, true, false)) {
            moves -= 100;
        } else {
            water.charges -= drink_from_hands( water );
            if( water.charges == water_charges ) {
                power_level += bionics["bio_evap"]->power_activate;
            }
        }
    } else if(bio.id == "bio_lighter") {
        if(!choose_adjacent(_("Start a fire where?"), dirx, diry) ||
           (!g->m.add_field(dirx, diry, fd_fire, 1))) {
            add_msg_if_player(m_info, _("You can't light a fire there."));
            power_level += bionics["bio_lighter"]->power_activate;
        }
    } else if(bio.id == "bio_leukocyte") {
        set_healthy(std::min(100, get_healthy() + 2));
        mod_healthy_mod(20);
    } else if(bio.id == "bio_geiger") {
        add_msg(m_info, _("Your radiation level: %d"), radiation);
    } else if(bio.id == "bio_radscrubber") {
        if (radiation > 4) {
            radiation -= 5;
        } else {
            radiation = 0;
        }
    } else if(bio.id == "bio_adrenaline") {
        if (has_effect("adrenaline")) {
            add_effect("adrenaline", 50);
        } else {
            add_effect("adrenaline", 200);
        }
    } else if(bio.id == "bio_blaster") {
        tmp_item = weapon;
        weapon = item("bio_blaster_gun", 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            power_level += bionics[bio.id]->power_activate;
        }
        weapon = tmp_item;
    } else if (bio.id == "bio_laser") {
        tmp_item = weapon;
        weapon = item("bio_laser_gun", 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            power_level += bionics[bio.id]->power_activate;
        }
        weapon = tmp_item;
    } else if(bio.id == "bio_chain_lightning") {
        tmp_item = weapon;
        weapon = item("bio_lightning", 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            power_level += bionics[bio.id]->power_activate;
        }
        weapon = tmp_item;
    } else if (bio.id == "bio_emp") {
        if(choose_adjacent(_("Create an EMP where?"), dirx, diry)) {
            g->emp_blast(dirx, diry);
        } else {
            power_level += bionics["bio_emp"]->power_activate;
        }
    } else if (bio.id == "bio_hydraulics") {
        add_msg(m_good, _("Your muscles hiss as hydraulic strength fills them!"));
        // Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        sounds::sound(posx(), posy(), 19, _("HISISSS!"));
    } else if (bio.id == "bio_water_extractor") {
        bool extracted = false;
        for( auto it = g->m.i_at(posx(), posy()).begin();
             it != g->m.i_at(posx(), posy()).end(); ++it) {
            if( it->is_corpse() ) {
                const int avail = it->get_var( "remaining_water", it->volume() / 2 );
                if(avail > 0 && query_yn(_("Extract water from the %s"), it->tname().c_str())) {
                    item water = item("water_clean", 0);
                    water.charges = avail;
                    if (g->handle_liquid(water, true, false)) {
                        moves -= 100;
                    } else {
                        water.charges -= drink_from_hands( water );
                    }
                    if( water.charges != avail ) {
                        extracted = true;
                        it->set_var( "remaining_water", static_cast<int>( water.charges ) );
                    }
                    break;
                }
            }
        }
        if (!extracted) {
            power_level += bionics["bio_water_extractor"]->power_activate;
        }
    } else if(bio.id == "bio_magnet") {
        for (int i = posx() - 10; i <= posx() + 10; i++) {
            for (int j = posy() - 10; j <= posy() + 10; j++) {
                if (g->m.i_at(i, j).size() > 0) {
                    int t; //not sure why map:sees really needs this, but w/e
                    if (g->m.sees(i, j, posx(), posy(), -1, t)) {
                        traj = line_to(i, j, posx(), posy(), t);
                    } else {
                        traj = line_to(i, j, posx(), posy(), 0);
                    }
                }
                traj.insert(traj.begin(), point(i, j));
                if( g->m.has_flag( "SEALED", i, j ) ) {
                    continue;
                }
                for (unsigned k = 0; k < g->m.i_at(i, j).size(); k++) {
                    tmp_item = g->m.i_at(i, j)[k];
                    if( (tmp_item.made_of("iron") || tmp_item.made_of("steel")) &&
                        tmp_item.weight() < weight_capacity() ) {
                        g->m.i_rem(i, j, k);
                        std::vector<point>::iterator it;
                        for (it = traj.begin(); it != traj.end(); ++it) {
                            int index = g->mon_at(it->x, it->y);
                            if (index != -1) {
                                g->zombie(index).apply_damage( this, bp_torso, tmp_item.weight() / 225 );
                                g->zombie(index).check_dead_state();
                                g->m.add_item_or_charges(it->x, it->y, tmp_item);
                                break;
                            } else if (g->m.move_cost(it->x, it->y) == 0) {
                                if (it != traj.begin()) {
                                    g->m.bash( it->x, it->y, tmp_item.weight() / 225 );
                                    if (g->m.move_cost(it->x, it->y) == 0) {
                                        g->m.add_item_or_charges((it - 1)->x, (it - 1)->y, tmp_item);
                                        break;
                                    }
                                } else {
                                    g->m.bash( it->x, it->y, tmp_item.weight() / 225 );
                                    if (g->m.move_cost(it->x, it->y) == 0) {
                                        break;
                                    }
                                }
                            }
                        }
                        if (it == traj.end()) {
                            g->m.add_item_or_charges(posx(), posy(), tmp_item);
                        }
                    }
                }
            }
        }
        moves -= 100;
    } else if(bio.id == "bio_lockpick") {
        item tmp_item( "pseuso_bio_picklock", 0 );
        if( invoke_item( &tmp_item ) == 0 ) {
            power_level += bionics["bio_lockpick"]->power_activate;
            return false;
        }
        if( tmp_item.damage > 0 ) {
            // TODO: damage the player / their bionics
        }
    } else if(bio.id == "bio_flashbang") {
        g->flashbang(posx(), posy(), true);
    } else if(bio.id == "bio_shockwave") {
        g->shockwave(posx(), posy(), 3, 4, 2, 8, true);
        add_msg_if_player(m_neutral, _("You unleash a powerful shockwave!"));
    } else if(bio.id == "bio_meteorologist") {
        // Calculate local wind power
        int vpart = -1;
        vehicle *veh = g->m.veh_at( posx(), posy(), vpart );
        int vehwindspeed = 0;
        if( veh ) {
            vehwindspeed = abs(veh->velocity / 100); // vehicle velocity in mph
        }
        const oter_id &cur_om_ter = overmap_buffer.ter(g->om_global_location());
        std::string omtername = otermap[cur_om_ter].name;
        int windpower = get_local_windpower(weatherPoint.windpower + vehwindspeed, omtername, g->is_sheltered(g->u.posx(), g->u.posy()));

        add_msg_if_player(m_info, _("Temperature: %s."), print_temperature(g->get_temperature()).c_str());
        add_msg_if_player(m_info, _("Relative Humidity: %s."), print_humidity(get_local_humidity(weatherPoint.humidity, g->weather, g->is_sheltered(g->u.posx(), g->u.posy()))).c_str());
        add_msg_if_player(m_info, _("Pressure: %s."), print_pressure((int)weatherPoint.pressure).c_str());
        add_msg_if_player(m_info, _("Wind Speed: %s."), print_windspeed((float)windpower).c_str());
        add_msg_if_player(m_info, _("Feels Like: %s."), print_temperature(get_local_windchill(weatherPoint.temperature, weatherPoint.humidity, windpower) + g->get_temperature()).c_str());
    } else if(bio.id == "bio_claws") {
        if (weapon.has_flag ("NO_UNWIELD")) {
            add_msg(m_info, _("Deactivate your %s first!"),
                    weapon.tname().c_str());
            power_level += bionics[bio.id]->power_activate;
            bio.powered = false;
            return false;
        } else if(weapon.type->id != "null") {
            add_msg(m_warning, _("Your claws extend, forcing you to drop your %s."),
                    weapon.tname().c_str());
            g->m.add_item_or_charges(posx(), posy(), weapon);
            weapon = item("bio_claws_weapon", 0);
            weapon.invlet = '#';
        } else {
            add_msg(m_neutral, _("Your claws extend!"));
            weapon = item("bio_claws_weapon", 0);
            weapon.invlet = '#';
        }
    } else if(bio.id == "bio_blade") {
        if (weapon.has_flag ("NO_UNWIELD")) {
            add_msg(m_info, _("Deactivate your %s first!"),
                    weapon.tname().c_str());
            power_level += bionics[bio.id]->power_activate;
            bio.powered = false;
            return false;
        } else if(weapon.type->id != "null") {
            add_msg(m_warning, _("Your blade extends, forcing you to drop your %s."),
                    weapon.tname().c_str());
            g->m.add_item_or_charges(posx(), posy(), weapon);
            weapon = item("bio_blade_weapon", 0);
            weapon.invlet = '#';
        } else {
            add_msg(m_neutral, _("You extend your blade!"));
            weapon = item("bio_blade_weapon", 0);
            weapon.invlet = '#';
        }
    } else if( bio.id == "bio_remote" ) {
        int choice = menu( true, _("Perform which function:"), _("Nothing"),
                           _("Control vehicle"), _("RC radio"), NULL );
        if( choice >= 2 && choice <= 3 ) {
            item ctr;
            if( choice == 2 ) {
                ctr = item( "remotevehcontrol", 0 );
            } else {
                ctr = item( "radiocontrol", 0 );
            }
            ctr.charges = power_level;
            int power_use = invoke_item( &ctr );
            power_level -= power_use;
            bio.powered = ctr.active;
        } else {
            bio.powered = g->remoteveh() != nullptr || get_value( "remote_controlling" ) != "";
        }
    }

    return true;
}

bool player::deactivate_bionic(int b, bool eff_only)
{
    bionic &bio = my_bionics[b];

    // Just do the effect, no stat changing or messages
    if (!eff_only) {
        if (!bio.powered) {
            // It's already off!
            return false;
        }
        if (!bionics[bio.id]->toggled) {
            // It's a fire-and-forget bionic, we can't turn it off but have to wait for it to run out of charge
            add_msg(m_info, _("You can't deactivate your %s manually!"), bionics[bio.id]->name.c_str());
            return false;
        }
        if (power_level < bionics[bio.id]->power_deactivate) {
            add_msg(m_info, _("You don't have the power to deactivate your %s."), bionics[bio.id]->name.c_str());
            return false;
        }

        //We can actually deactivate now, do deactivation-y things
        power_level -= bionics[bio.id]->power_deactivate;
        bio.powered = false;
        add_msg(m_neutral, _("You deactivate your %s."), bionics[bio.id]->name.c_str());
    }

    // Deactivation effects go here
    if (bio.id == "bio_cqb") {
        // check if player knows current style naturally, otherwise drop them back to style_none
        if (style_selected != "style_none") {
            bool has_style = false;
            for( auto &elem : ma_styles ) {
                if( elem == style_selected ) {
                    has_style = true;
                }
            }
            if (!has_style) {
                style_selected = "style_none";
            }
        }
    } else if(bio.id == "bio_claws") {
        if (weapon.type->id == "bio_claws_weapon") {
            add_msg(m_neutral, _("You withdraw your claws."));
            weapon = ret_null;
        }
    } else if(bio.id == "bio_blade") {
        if (weapon.type->id == "bio_blade_weapon") {
            add_msg(m_neutral, _("You retract your blade."));
            weapon = ret_null;
        }
    } else if( bio.id == "bio_remote" ) {
        if( g->remoteveh() != nullptr && !has_active_item( "remotevehcontrol" ) ) {
            g->setremoteveh( nullptr );
        } else if( get_value( "remote_controlling" ) != "" && !has_active_item( "radiocontrol" ) ) {
            set_value( "remote_controlling", "" );
        }
    }

    return true;
}

void player::process_bionic(int b)
{
    bionic &bio = my_bionics[b];
    if (!bio.powered) {
        // Only powered bionics should be processed
        return;
    }

    if (bio.charge > 0) {
        // Units already with charge just lose charge
        bio.charge--;
    } else {
        if (bionics[bio.id]->charge_time > 0) {
            // Try to recharge our bionic if it is made for it
            if (bionics[bio.id]->power_over_time > 0) {
                if (power_level < bionics[bio.id]->power_over_time) {
                    // No power to recharge, so deactivate
                    bio.powered = false;
                    add_msg(m_neutral, _("Your %s powers down."), bionics[bio.id]->name.c_str());
                    // This purposely bypasses the deactivation cost
                    deactivate_bionic(b, true);
                    return;
                } else {
                    // Pay the recharging cost
                    power_level -= bionics[bio.id]->power_over_time;
                    // We just spent our first turn of charge, so -1 here
                    bio.charge = bionics[bio.id]->charge_time - 1;
                }
            // Some bionics are a 1-shot activation so they just deactivate at 0 charge.
            } else {
                bio.powered = false;
                add_msg(m_neutral, _("Your %s powers down."), bionics[bio.id]->name.c_str());
                // This purposely bypasses the deactivation cost
                deactivate_bionic(b, true);
                return;
            }
        }
    }

    // Bionic effects on every turn they are active go here.
    if (bio.id == "bio_night") {
        if (calendar::turn % 5) {
            add_msg(m_neutral, _("Artificial night generator active!"));
        }
    } else if( bio.id == "bio_remote" ) {
        if( g->remoteveh() == nullptr && get_value( "remote_controlling" ) == "" ) {
            bio.powered = false;
            add_msg( m_warning, _("Your %s has lost connection and is turning off."), 
                     bionics[bio.id]->name.c_str() );
        }
    }
}

void bionics_uninstall_failure(player *u)
{
    switch (rng(1, 5)) {
    case 1:
        add_msg(m_neutral, _("You flub the removal."));
        break;
    case 2:
        add_msg(m_neutral, _("You mess up the removal."));
        break;
    case 3:
        add_msg(m_neutral, _("The removal fails."));
        break;
    case 4:
        add_msg(m_neutral, _("The removal is a failure."));
        break;
    case 5:
        add_msg(m_neutral, _("You screw up the removal."));
        break;
    }
    add_msg(m_bad, _("Your body is severely damaged!"));
    u->hurtall(rng(30, 80), u); // stop hurting yourself!
}

// bionic manipulation chance of success
int bionic_manip_cos(int p_int, int s_electronics, int s_firstaid, int s_mechanics,
                     int bionic_difficulty)
{
    int pl_skill = p_int * 4 +
                   s_electronics * 4 +
                   s_firstaid    * 3 +
                   s_mechanics   * 1;

    // Medical residents have some idea what they're doing
    if (g->u.has_trait("PROF_MED")) {
        pl_skill += 3;
        add_msg(m_neutral, _("You prep yourself to begin surgery."));
    }

    // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = float (pl_skill) - std::min( float (40),
                           float (pl_skill) - float (pl_skill) / float (10.0));

    // we will base chance_of_success on a ratio of skill and difficulty
    // when skill=difficulty, this gives us 1.  skill < difficulty gives a fraction.
    float skill_difficulty_parameter = float(adjusted_skill / (4.0 * bionic_difficulty));

    // when skill == difficulty, chance_of_success is 50%. Chance of success drops quickly below that
    // to reserve bionics for characters with the appropriate skill.  For more difficult bionics, the
    // curve flattens out just above 80%
    int chance_of_success = int((100 * skill_difficulty_parameter) /
                                (skill_difficulty_parameter + sqrt( 1 / skill_difficulty_parameter)));

    return chance_of_success;
}

bool player::uninstall_bionic(bionic_id b_id)
{
    // malfunctioning bionics don't have associated items and get a difficulty of 12
    int difficulty = 12;
    if( item::type_is_defined( b_id ) ) {
        auto type = item::find_type( b_id );
        if( type->bionic ) {
            difficulty = type->bionic->difficulty;
        }
    }

    if (!has_bionic(b_id)) {
        popup(_("You don't have this bionic installed."));
        return false;
    }
    if (!(has_items_with_quality("CUT", 1, 1) && has_amount("1st_aid", 1))) {
        popup(_("Removing bionics requires a cutting tool and a first aid kit."));
        return false;
    }

    if ( b_id == "bio_blaster" ) {
        popup(_("Removing your Fusion Blaster Arm would leave you with a useless stump."));
        return false;
    }

    // removal of bionics adds +2 difficulty over installation
    int chance_of_success = bionic_manip_cos(int_cur,
                            skillLevel("electronics"),
                            skillLevel("firstaid"),
                            skillLevel("mechanics"),
                            difficulty + 2);

    if (!query_yn(_("WARNING: %i percent chance of failure and SEVERE bodily damage! Remove anyway?"),
                  100 - chance_of_success)) {
        return false;
    }

    use_charges("1st_aid", 1);

    practice( "electronics", int((100 - chance_of_success) * 1.5) );
    practice( "firstaid", int((100 - chance_of_success) * 1.0) );
    practice( "mechanics", int((100 - chance_of_success) * 0.5) );

    int success = chance_of_success - rng(1, 100);

    if (success > 0) {
        add_memorial_log(pgettext("memorial_male", "Removed bionic: %s."),
                         pgettext("memorial_female", "Removed bionic: %s."),
                         bionics[b_id]->name.c_str());
        // until bionics can be flagged as non-removable
        add_msg(m_neutral, _("You jiggle your parts back into their familiar places."));
        add_msg(m_good, _("Successfully removed %s."), bionics[b_id]->name.c_str());
        remove_bionic(b_id);
        g->m.spawn_item(posx(), posy(), "burnt_out_bionic", 1);
    } else {
        add_memorial_log(pgettext("memorial_male", "Removed bionic: %s."),
                         pgettext("memorial_female", "Removed bionic: %s."),
                         bionics[b_id]->name.c_str());
        bionics_uninstall_failure(this);
    }
    g->refresh_all();
    return true;
}

bool player::install_bionics(const itype &type)
{
    if( type.bionic.get() == nullptr ) {
        debugmsg("Tried to install NULL bionic");
        return false;
    }
    const std::string bioid = type.bionic->bionic_id;
    if( bionics.count( bioid ) == 0 ) {
        popup("invalid / unknown bionic id %s", bioid.c_str());
        return false;
    }
    if( has_bionic( bioid ) ) {
        if( !( bioid == "bio_power_storage" || bioid == "bio_power_storage_mkII" ) ) {
            popup(_("You have already installed this bionic."));
            return false;
        }
    }
    const int difficult = type.bionic->difficulty;
    int chance_of_success = bionic_manip_cos(int_cur,
                            skillLevel("electronics"),
                            skillLevel("firstaid"),
                            skillLevel("mechanics"),
                            difficult);

    if (!query_yn(
            _("WARNING: %i percent chance of genetic damage, blood loss, or damage to existing bionics! Install anyway?"),
            100 - chance_of_success)) {
        return false;
    }
    int pow_up = 0;
    if( bioid == "bio_power_storage" ) {
        pow_up = BATTERY_AMOUNT;
    } else if( bioid == "bio_power_storage_mkII" ) {
        pow_up = 250;
    }

    practice( "electronics", int((100 - chance_of_success) * 1.5) );
    practice( "firstaid", int((100 - chance_of_success) * 1.0) );
    practice( "mechanics", int((100 - chance_of_success) * 0.5) );
    int success = chance_of_success - rng(0, 99);
    if (success > 0) {
        add_memorial_log(pgettext("memorial_male", "Installed bionic: %s."),
                         pgettext("memorial_female", "Installed bionic: %s."),
                         bionics[bioid]->name.c_str());
        if (pow_up) {
            max_power_level += pow_up;
            add_msg_if_player(m_good, _("Increased storage capacity by %i"), pow_up);
        } else {
            add_msg(m_good, _("Successfully installed %s."), bionics[bioid]->name.c_str());
            add_bionic(bioid);
        }
    } else {
        add_memorial_log(pgettext("memorial_male", "Installed bionic: %s."),
                         pgettext("memorial_female", "Installed bionic: %s."),
                         bionics[bioid]->name.c_str());
        bionics_install_failure(this, difficult, success);
    }
    g->refresh_all();
    return true;
}

void bionics_install_failure(player *u, int difficulty, int success)
{
    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful install.  We use this to determine consequences for failing.
    success = abs(success);

    // it would be better for code reuse just to pass in skill as an argument from install_bionic
    // pl_skill should be calculated the same as in install_bionics
    int pl_skill = u->int_cur * 4 +
                   u->skillLevel("electronics") * 4 +
                   u->skillLevel("firstaid")    * 3 +
                   u->skillLevel("mechanics")   * 1;
    // Medical residents get a substantial assist here
    if (u->has_trait("PROF_MED")) {
        pl_skill += 6;
    }

    // for failure_level calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = float (pl_skill) - std::min( float (40),
                           float (pl_skill) - float (pl_skill) / float (10.0));

    // failure level is decided by how far off the character was from a successful install, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    int failure_level = int(sqrt(success * 4.0 * difficulty / float (adjusted_skill)));
    int fail_type = (failure_level > 5 ? 5 : failure_level);

    if (fail_type <= 0) {
        add_msg(m_neutral, _("The installation fails without incident."));
        return;
    }

    switch (rng(1, 5)) {
    case 1:
        add_msg(m_neutral, _("You flub the installation."));
        break;
    case 2:
        add_msg(m_neutral, _("You mess up the installation."));
        break;
    case 3:
        add_msg(m_neutral, _("The installation fails."));
        break;
    case 4:
        add_msg(m_neutral, _("The installation is a failure."));
        break;
    case 5:
        add_msg(m_neutral, _("You screw up the installation."));
        break;
    }

    if (u->has_trait("PROF_MED")) {
    //~"Complications" is USian medical-speak for "unintended damage from a medical procedure".
        add_msg(m_neutral, _("Your training helps you minimize the complications."));
    // In addition to the bonus, medical residents know enough OR protocol to avoid botching.
    // Take MD and be immune to faulty bionics.
        if (fail_type == 5) {
            fail_type = rng(1,3);
        }
    }

    if (fail_type == 3 && u->num_bionics() == 0) {
        fail_type = 2;    // If we have no bionics, take damage instead of losing some
    }

    switch (fail_type) {

    case 1:
        if (!(u->has_trait("NOPAIN"))) {
            add_msg(m_bad, _("It really hurts!"));
            u->mod_pain( rng(failure_level * 3, failure_level * 6) );
        }
        break;

    case 2:
        add_msg(m_bad, _("Your body is damaged!"));
        u->hurtall(rng(failure_level, failure_level * 2), u); // you hurt yourself
        break;

    case 3:
        if (u->num_bionics() <= failure_level && u->max_power_level == 0) {
            add_msg(m_bad, _("All of your existing bionics are lost!"));
        } else {
            add_msg(m_bad, _("Some of your existing bionics are lost!"));
        }
        for (int i = 0; i < failure_level && u->remove_random_bionic(); i++) {
            ;
        }
        break;

    case 4:
        add_msg(m_mixed, _("You do damage to your genetics, causing mutation!"));
        while (failure_level > 0) {
            u->mutate();
            failure_level -= rng(1, failure_level + 2);
        }
        break;

    case 5: {
        add_msg(m_bad, _("The installation is faulty!"));
        std::vector<bionic_id> valid;
        for( auto &faulty_bionic : faulty_bionics ) {
            if( !u->has_bionic( faulty_bionic ) ) {
                valid.push_back( faulty_bionic );
            }
        }
        if (valid.empty()) { // We've got all the bad bionics!
            if (u->max_power_level > 0) {
                int old_power = u->max_power_level;
                add_msg(m_bad, _("You lose power capacity!"));
                u->max_power_level = rng(0, u->max_power_level - 25);
                u->add_memorial_log(pgettext("memorial_male", "Lost %d units of power capacity."),
                                      pgettext("memorial_female", "Lost %d units of power capacity."),
                                      old_power - u->max_power_level);
            }
            // TODO: What if we can't lose power capacity?  No penalty?
        } else {
            int index = rng(0, valid.size() - 1);
            u->add_bionic(valid[index]);
            u->add_memorial_log(pgettext("memorial_male", "Installed bad bionic: %s."),
                                pgettext("memorial_female", "Installed bad bionic: %s."),
                                bionics[valid[index]]->name.c_str());
        }
    }
    break;
    }
}

void reset_bionics()
{
    for( auto &bionic : bionics ) {
        delete bionic.second;
    }
    bionics.clear();
    faulty_bionics.clear();
    power_source_bionics.clear();
    unpowered_bionics.clear();
}

void load_bionic(JsonObject &jsobj)
{
    std::string id = jsobj.get_string("id");
    std::string name = _(jsobj.get_string("name").c_str());
    std::string description = _(jsobj.get_string("description").c_str());
    int on_cost = jsobj.get_int("act_cost", 0);

    bool toggled = jsobj.get_bool("toggled", false);
    // Requires ability to toggle
    int off_cost = jsobj.get_int("deact_cost", 0);

    int time = jsobj.get_int("time", 0);
    // Requires a non-zero time
    int react_cost = jsobj.get_int("react_cost", 0);

    bool faulty = jsobj.get_bool("faulty", false);
    bool power_source = jsobj.get_bool("power_source", false);

    if (faulty) {
        faulty_bionics.push_back(id);
    }
    if (power_source) {
        power_source_bionics.push_back(id);
    }
    // Things are activatable if they are toggleable, have a activation cost, or have a activation time
    if (!toggled && on_cost <= 0 && time <= 0) {
        unpowered_bionics.push_back(id);
    }

    bionics[id] = new bionic_data(name, power_source, toggled, on_cost, off_cost, react_cost, time, description, faulty);
}
