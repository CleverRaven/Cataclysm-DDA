#include "player.h"
#include "game.h"
#include "rng.h"
#include "input.h"
#include "item.h"
#include "bionics.h"
#include "line.h"
#include "json.h"
#include <math.h>    //sqrt
#include <algorithm> //std::min

#define BATTERY_AMOUNT 4 // How much batteries increase your power

std::map<bionic_id, bionic_data *> bionics;
std::vector<bionic_id> faulty_bionics;
std::vector<bionic_id> power_source_bionics;
std::vector<bionic_id> unpowered_bionics;

void bionics_install_failure(player *u, it_bionic *type, int success);

bionic_data::bionic_data(std::string new_name, bool new_power_source, bool new_activated,
                         int new_power_cost, int new_charge_time, std::string new_description,
                         bool new_faulty) : description(new_description)
{
    name = new_name;
    power_source = new_power_source;
    activated = new_activated;
    power_cost = new_power_cost;
    charge_time = new_charge_time;
    faulty = new_faulty;
}

bionic_id game::random_good_bionic() const
{
    std::map<std::string, bionic_data *>::const_iterator random_bionic;
    do {
        random_bionic = bionics.begin();
        std::advance(random_bionic, rng(0, bionics.size() - 1));
    } while (random_bionic->first == "bio_null" || random_bionic->second->faulty);
    // TODO: remove bio_null
    return random_bionic->first;
}

void show_bionics_titlebar(WINDOW *window, player *p, bool activating, bool reassigning)
{
    werase(window);

    std::string caption = _("BIONICS -");
    int cap_offset = utf8_width(caption.c_str()) + 1;
    mvwprintz(window, 0,  0, c_blue, "%s", caption.c_str());

    std::stringstream pwr;
    pwr << _("Power: ") << int(p->power_level) << _("/") << int(p->max_power_level);
    int pwr_length = utf8_width(pwr.str().c_str()) + 1;
    mvwprintz(window, 0, getmaxx(window) - pwr_length, c_white, "%s", pwr.str().c_str());

    std::string desc;
    int desc_length = getmaxx(window) - cap_offset - pwr_length;

    if(reassigning) {
        desc = _("Reassigning.\nSelect a bionic to reassign or press SPACE to cancel.");
    } else if(activating) {
        desc = _("Activating. Press <color_yellow>!</color> to examine your implants.\nPress <color_yellow>=</color> to reassign a key.");
    } else {
        desc = _("Examining. Press <color_yellow>!</color> to activate your implants.\nPress <color_yellow>=</color> to reassign a key.");
    }
    fold_and_print(window, 0, cap_offset, desc_length, c_white, desc);

    wrefresh(window);
}

void player::power_bionics()
{
    int HEIGHT = TERMY;
    int WIDTH = FULL_SCREEN_WIDTH;
    int START_X = (TERMX - WIDTH) / 2;
    int START_Y = (TERMY - HEIGHT) / 2;
    WINDOW *wBio = newwin(HEIGHT, WIDTH, START_Y, START_X);
    int DESCRIPTION_WIDTH = WIDTH - 2; // Same width as bionics window minus 2 for the borders
    int DESCRIPTION_HEIGHT = 5;
    int DESCRIPTION_START_X = getbegx(wBio) + 1; // +1 to avoid border
    int DESCRIPTION_START_Y = getmaxy(wBio) - DESCRIPTION_HEIGHT -
                              1; // At the bottom of the bio window, -1 to avoid border
    WINDOW *w_description = newwin(DESCRIPTION_HEIGHT, DESCRIPTION_WIDTH, DESCRIPTION_START_Y,
                                   DESCRIPTION_START_X);

    int TITLE_WIDTH = DESCRIPTION_WIDTH;
    int TITLE_HEIGHT = 2;
    int TITLE_START_X = DESCRIPTION_START_X;
    int TITLE_START_Y = START_Y + 1;
    WINDOW *w_title = newwin(TITLE_HEIGHT, TITLE_WIDTH, TITLE_START_Y, TITLE_START_X);

    int scroll_position = 0;
    bool redraw = true;
    bool reassigning = false;
    bool activating = true; // examination mode if activating = false and reassigning = false

    std::vector <bionic *> passive;
    std::vector <bionic *> active;
    for (size_t i = 0; i < my_bionics.size(); i++) {
        if (!bionics[my_bionics[i].id]->activated) {
            passive.push_back(&my_bionics[i]);
        } else {
            active.push_back(&my_bionics[i]);
        }
    }

    int HEADER_LINE_Y = TITLE_START_Y + TITLE_HEIGHT; // + lines with text in titlebar
    int DESCRIPTION_LINE_Y = DESCRIPTION_START_Y - 1;

    // maximal number of rows in both columns
    const int bionic_count = std::max(passive.size(), active.size());
    // number of rows with bionics shown (+1 for displaying "Passive:"/"Active:")
    const int bionic_display_height = DESCRIPTION_LINE_Y - HEADER_LINE_Y - 2;
    int max_scroll_position = bionic_count - bionic_display_height;
    int second_column = 32;

    while(true) {
        // offset for display: bionic with index i is drawn at y=list_start_y+i
        // the header ("Passive:"/"Active:") is drawn at y=HEADER_LINE_Y+1
        // drawing the bionics starts with bionic[scroll_position]
        const int list_start_y = HEADER_LINE_Y + 2 - scroll_position;
        if(redraw) {
            redraw = false;

            werase(wBio);
            draw_border(wBio);

            draw_exam_window(wBio, DESCRIPTION_LINE_Y, (!reassigning && !activating));
            for (int i = 1; i < WIDTH - 1; i++) {
                mvwputch(wBio, HEADER_LINE_Y, i, BORDER_COLOR, LINE_OXOX); // Draw line under title
            }

            // Draw symbols to connect additional lines to border
            mvwputch(wBio, HEADER_LINE_Y, 0, BORDER_COLOR, LINE_XXXO); // |-
            mvwputch(wBio, HEADER_LINE_Y, WIDTH - 1, BORDER_COLOR, LINE_XOXX); // -|

            nc_color type;
            mvwprintz(wBio, HEADER_LINE_Y + 1, 2, c_ltblue, _("Passive:"));
            if (passive.size() <= 0) {
                mvwprintz(wBio, list_start_y, 2, c_ltgray, _("None"));
            } else {
                for (size_t i = scroll_position; i < passive.size(); i++) {
                    if (list_start_y + i == ((!activating && !reassigning) ?
                                             DESCRIPTION_LINE_Y : HEIGHT - 1)) {
                        break;
                    }
                    if (bionics[passive[i]->id]->power_source) {
                        type = c_ltcyan;
                    } else {
                        type = c_cyan;
                    }
                    mvwprintz(wBio, list_start_y + i, 2, type, "%c %s", passive[i]->invlet, bionics[passive[i]->id]->name.c_str());
                }
            }

            mvwprintz(wBio, HEADER_LINE_Y + 1, second_column, c_ltblue, _("Active:"));
            if (active.size() <= 0) {
                mvwprintz(wBio, list_start_y, second_column, c_ltgray, _("None"));
            } else {
                for (size_t i = scroll_position; i < active.size(); i++) {
                    if (list_start_y + i == ((!activating && !reassigning) ?
                                             DESCRIPTION_LINE_Y : HEIGHT - 1)) {
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
                    mvwprintz(wBio, list_start_y + i, second_column + 2, type,
                              (active[i]->powered ? _("%s - ON") : _("%s - %d PU / %d trns")),
                              bionics[active[i]->id]->name.c_str(),
                              bionics[active[i]->id]->power_cost,
                              bionics[active[i]->id]->charge_time);
                }
            }

            if(scroll_position > 0) {
                mvwputch(wBio, HEADER_LINE_Y + 2, 0, c_ltgreen, '^');
            }
            if(scroll_position < max_scroll_position && max_scroll_position > 0) {
                mvwputch(wBio, (!activating && !reassigning) ? ((HEADER_LINE_Y + 2) +
                         bionic_display_height - 1) : (HEIGHT - 2), 0, c_ltgreen, 'v');
            }
        }
        wrefresh(wBio);
        show_bionics_titlebar(w_title, this, activating, reassigning);
        long ch = getch();
        bionic *tmp = NULL;
        if (reassigning) {
            reassigning = false;
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
        } else if (ch == KEY_DOWN) {
            if(scroll_position < max_scroll_position) {
                scroll_position++;
                redraw = true;
            }
        } else if (ch == KEY_UP) {
            if(scroll_position > 0) {
                scroll_position--;
                redraw = true;
            }
        } else if (ch == '=') {
            reassigning = true;
        } else if (ch == '!') {
            activating = !activating;
            if (!reassigning && !activating) { // examination mode
                werase(w_description);
            }
            draw_exam_window(wBio, DESCRIPTION_LINE_Y, (!reassigning && !activating));
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
            if (activating) {
                if (bio_data.activated) {
                    itype_id weapon_id = weapon.type->id;
                    if (tmp->powered) {
                        tmp->powered = false;
                        g->add_msg(_("%s powered off."), bio_data.name.c_str());
                    } else if (power_level >= bio_data.power_cost ||
                               (weapon_id == "bio_claws_weapon" && bio_id == "bio_claws_weapon") ||
                               (weapon_id == "bio_blade_weapon" && bio_id == "bio_blade_weapon")) {
                        int b = tmp - &my_bionics[0];

                        //this will clear the bionics menu for targeting purposes
                        werase(wBio);
                        wrefresh(wBio);
                        delwin(w_title);
                        delwin(w_description);
                        delwin(wBio);
                        g->draw();
                        activate_bionic(b);
                    }
                    // Action done, leave screen
                    break;
                } else {
                    popup(_("\
You can not activate %s!  To read a description of \
%s, press '!', then '%c'."), bio_data.name.c_str(),
                          bio_data.name.c_str(), tmp->invlet);
                    redraw = true;
                }
            } else { // Describing bionics, not activating them!
                draw_exam_window(wBio, DESCRIPTION_LINE_Y, true);
                // Clear the lines first
                werase(w_description);
                fold_and_print(w_description, 0, 0, WIDTH - 2, c_ltblue, bio_data.description);
                wrefresh(w_description);
            }
        }
    }
    //if we activated a bionic, already killed the windows
    if(!activating){
        werase(wBio);
        wrefresh(wBio);
        delwin(w_title);
        delwin(w_description);
        delwin(wBio);
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
void player::activate_bionic(int b)
{
    bionic bio = my_bionics[b];
    int power_cost = bionics[bio.id]->power_cost;
    if ((weapon.type->id == "bio_claws_weapon" && bio.id == "bio_claws_weapon") ||
    (weapon.type->id == "bio_blade_weapon" && bio.id == "bio_blade_weapon")) {
        power_cost = 0;
    }
    if (power_level < power_cost) {
        if (my_bionics[b].powered) {
            g->add_msg(_("Your %s powers down."), bionics[bio.id]->name.c_str());
            my_bionics[b].powered = false;
        } else {
            g->add_msg(_("You cannot power your %s"), bionics[bio.id]->name.c_str());
        }
        return;
    }

    if (my_bionics[b].powered && my_bionics[b].charge > 0) {
        // Already-on units just lose a bit of charge
        my_bionics[b].charge--;
    } else {
        // Not-on units, or those with zero charge, have to pay the power cost
        if (bionics[bio.id]->charge_time > 0) {
            my_bionics[b].powered = true;
            my_bionics[b].charge = bionics[bio.id]->charge_time - 1;
        }
        power_level -= power_cost;
    }

    std::string junk;
    std::vector<point> traj;
    std::vector<std::string> good;
    std::vector<std::string> bad;
    int dirx, diry;
    item tmp_item;

    if(bio.id == "bio_painkiller") {
        pkill += 6;
        pain -= 2;
        if (pkill > pain) {
            pkill = pain;
        }
    } else if (bio.id == "bio_nanobots") {
        rem_disease("bleed");
        healall(4);
    } else if (bio.id == "bio_night") {
        if (g->turn % 5) {
            g->add_msg(_("Artificial night generator active!"));
        }
    } else if (bio.id == "bio_resonator") {
        g->sound(posx, posy, 30, _("VRRRRMP!"));
        for (int i = posx - 1; i <= posx + 1; i++) {
            for (int j = posy - 1; j <= posy + 1; j++) {
                g->m.bash(i, j, 40, junk);
                g->m.bash(i, j, 40, junk); // Multibash effect, so that doors &c will fall
                g->m.bash(i, j, 40, junk);
                if (g->m.is_destructable(i, j) && rng(1, 10) >= 4) {
                    g->m.ter_set(i, j, t_rubble);
                }
            }
        }
    } else if (bio.id == "bio_time_freeze") {
        moves += 100 * power_level;
        power_level = 0;
        g->add_msg(_("Your speed suddenly increases!"));
        if (one_in(3)) {
            g->add_msg(_("Your muscles tear with the strain."));
            hurt(bp_arms, 0, rng(5, 10));
            hurt(bp_arms, 1, rng(5, 10));
            hurt(bp_legs, 0, rng(7, 12));
            hurt(bp_legs, 1, rng(7, 12));
            hurt(bp_torso, -1, rng(5, 15));
        }
        if (one_in(5)) {
            add_disease("teleglow", rng(50, 400));
        }
    } else if (bio.id == "bio_teleport") {
        g->teleport();
        add_disease("teleglow", 300);
    }
    // TODO: More stuff here (and bio_blood_filter)
    else if(bio.id == "bio_blood_anal") {
        WINDOW *w = newwin(20, 40, 3 + ((TERMY > 25) ? (TERMY - 25) / 2 : 0),
                           10 + ((TERMX > 80) ? (TERMX - 80) / 2 : 0));
        draw_border(w);
        if (has_disease("fungus")) {
            bad.push_back(_("Fungal Parasite"));
        }
        if (has_disease("dermatik")) {
            bad.push_back(_("Insect Parasite"));
        }
        if (has_effect("poison")) {
            bad.push_back(_("Poison"));
        }
        if (radiation > 0) {
            bad.push_back(_("Irradiated"));
        }
        if (has_disease("pkill1")) {
            good.push_back(_("Minor Painkiller"));
        }
        if (has_disease("pkill2")) {
            good.push_back(_("Moderate Painkiller"));
        }
        if (has_disease("pkill3")) {
            good.push_back(_("Heavy Painkiller"));
        }
        if (has_disease("pkill_l")) {
            good.push_back(_("Slow-Release Painkiller"));
        }
        if (has_disease("drunk")) {
            good.push_back(_("Alcohol"));
        }
        if (has_disease("cig")) {
            good.push_back(_("Nicotine"));
        }
        if (has_disease("meth")) {
            good.push_back(_("Methamphetamines"));
        }
        if (has_disease("high")) {
            good.push_back(_("Intoxicant: Other"));
        }
        if (has_disease("weed_high")) {
            good.push_back(_("THC Intoxication"));
        }
        if (has_disease("hallu") || has_disease("visuals")) {
            bad.push_back(_("Magic Mushroom"));
        }
        if (has_disease("iodine")) {
            good.push_back(_("Iodine"));
        }
        if (has_disease("took_xanax")) {
            good.push_back(_("Xanax"));
        }
        if (has_disease("took_prozac")) {
            good.push_back(_("Prozac"));
        }
        if (has_disease("took_flumed")) {
            good.push_back(_("Antihistamines"));
        }
        if (has_disease("adrenaline")) {
            good.push_back(_("Adrenaline Spike"));
        }
        if (has_disease("tapeworm")) {  // This little guy is immune to the blood filter though, as he lives in your bowels.
            good.push_back(_("Intestinal Parasite"));
        }
        if (has_disease("bloodworms")) {
            good.push_back(_("Hemolytic Parasites"));
        }
        if (has_disease("brainworm")) {  // This little guy is immune to the blood filter too, as he lives in your brain.
            good.push_back(_("Intracranial Parasite"));
        }
        if (has_disease("paincysts")) {  // These little guys are immune to the blood filter too, as they lives in your muscles.
            good.push_back(_("Intramuscular Parasites"));
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
        g->add_msg(_("You activate your blood filtration system."));
        rem_disease("fungus");
        rem_disease("dermatik");
        rem_disease("bloodworms");
        remove_effect("poison");
        rem_disease("pkill1");
        rem_disease("pkill2");
        rem_disease("pkill3");
        rem_disease("pkill_l");
        rem_disease("drunk");
        rem_disease("cig");
        rem_disease("high");
        rem_disease("hallu");
        rem_disease("visuals");
        rem_disease("iodine");
        rem_disease("took_xanax");
        rem_disease("took_prozac");
        rem_disease("took_flumed");
        rem_disease("adrenaline");
        rem_disease("meth");
        pkill = 0;
        stim = 0;
    } else if(bio.id == "bio_evap") {
        item water = item(itypes["water_clean"], 0);
        if (g->handle_liquid(water, true, true)) {
            moves -= 100;
        } else if (query_yn(_("Drink from your hands?"))) {
            inv.push_back(water);
            consume(inv.position_by_type(water.typeId()));
            moves -= 350;
        } else {
            power_level += bionics["bio_evap"]->power_cost;
        }
    } else if(bio.id == "bio_lighter") {
        if(!g->choose_adjacent(_("Start a fire where?"), dirx, diry) ||
           (!g->m.add_field(dirx, diry, fd_fire, 1))) {
            g->add_msg_if_player(this, _("You can't light a fire there."));
            power_level += bionics["bio_lighter"]->power_cost;
        }

    }
    if(bio.id == "bio_leukocyte") {
        g->add_msg(_("You activate your leukocyte breeder system."));
        if (health < 0) {
            health = 0;
        } else {
            health += 5;
        }
    }
    if(bio.id == "bio_geiger") {
        g->add_msg(_("Your radiation level: %d"), radiation);
    }
    if(bio.id == "bio_radscrubber") {
        g->add_msg(_("You activate your radiation scrubber system."));
          if (radiation > 4) {
            radiation -= 5;
        } else {
            radiation = 0;
        }
    }
    if(bio.id == "bio_adrenaline") {
            g->add_msg(_("You activate your adrenaline pump."));
      if (has_disease("adrenaline")) {
            add_disease("adrenaline", 50);
  } else {
    add_disease("adrenaline", 200);
  }
    } else if(bio.id == "bio_claws") {
        if (weapon.type->id == "bio_claws_weapon") {
            g->add_msg(_("You withdraw your claws."));
            weapon = ret_null;
        } else if (weapon.has_flag ("NO_UNWIELD")) {
            g->add_msg(_("Deactivate your %s first!"),
                       weapon.tname().c_str());
            power_level += bionics[bio.id]->power_cost;
            return;
        } else if(weapon.type->id != "null") {
            g->add_msg(_("Your claws extend, forcing you to drop your %s."),
                       weapon.tname().c_str());
            g->m.add_item_or_charges(posx, posy, weapon);
            weapon = item(itypes["bio_claws_weapon"], 0);
            weapon.invlet = '#';
        } else {
            g->add_msg(_("Your claws extend!"));
            weapon = item(itypes["bio_claws_weapon"], 0);
            weapon.invlet = '#';
        }
    } else if(bio.id == "bio_blade") {
        if (weapon.type->id == "bio_blade_weapon") {
            g->add_msg(_("You retract your blade."));
            weapon = ret_null;
        } else if (weapon.has_flag ("NO_UNWIELD")) {
            g->add_msg(_("Deactivate your %s first!"),
                       weapon.tname().c_str());
            power_level += bionics[bio.id]->power_cost;
            return;
        } else if(weapon.type->id != "null") {
            g->add_msg(_("Your blade extends, forcing you to drop your %s."),
                       weapon.tname().c_str());
            g->m.add_item_or_charges(posx, posy, weapon);
            weapon = item(itypes["bio_blade_weapon"], 0);
            weapon.invlet = '#';
        } else {
            g->add_msg(_("You extend your blade!"));
            weapon = item(itypes["bio_blade_weapon"], 0);
            weapon.invlet = '#';
        }
    } else if(bio.id == "bio_blaster") {
        tmp_item = weapon;
        weapon = item(itypes["bio_blaster_gun"], 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            power_level += bionics[bio.id]->power_cost;
        }
        weapon = tmp_item;
    } else if (bio.id == "bio_laser") {
        tmp_item = weapon;
        weapon = item(itypes["bio_laser_gun"], 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            power_level += bionics[bio.id]->power_cost;
        }
        weapon = tmp_item;
    } else if(bio.id == "bio_chain_lightning") {
        tmp_item = weapon;
        weapon = item(itypes["bio_lightning"], 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            power_level += bionics[bio.id]->power_cost;
        }
        weapon = tmp_item;
    } else if (bio.id == "bio_emp") {
        if(g->choose_adjacent(_("Create an EMP where?"), dirx, diry)) {
            g->emp_blast(dirx, diry);
        } else {
            power_level += bionics["bio_emp"]->power_cost;
        }
    } else if (bio.id == "bio_hydraulics") {
        g->add_msg(_("Your muscles hiss as hydraulic strength fills them!"));
        // Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        g->sound(posx, posy, 19, _("HISISSS!"));
    } else if (bio.id == "bio_water_extractor") {
        bool extracted = false;
        for (unsigned i = 0; i < g->m.i_at(posx, posy).size(); i++) {
            item &tmp = g->m.i_at(posx, posy)[i];
            if (tmp.type->id == "corpse" ) {
                int avail = 0;
                if ( tmp.item_vars.find("remaining_water") != tmp.item_vars.end() ) {
                    avail = atoi ( tmp.item_vars["remaining_water"].c_str() );
                } else {
                    avail = tmp.volume() / 2;
                }
                if(avail > 0 && query_yn(_("Extract water from the %s"), tmp.tname().c_str())) {
                    item water = item(itypes["water_clean"], 0);
                    if (g->handle_liquid(water, true, true)) {
                        moves -= 100;
                    } else if (query_yn(_("Drink directly from the condensor?"))) {
                        inv.push_back(water);
                        consume(inv.position_by_type(water.typeId()));
                        moves -= 350;
                    }
                    extracted = true;
                    avail--;
                    tmp.item_vars["remaining_water"] = string_format("%d", avail);
                    break;
                }
            }
        }
        if (!extracted) {
            power_level += bionics["bio_water_extractor"]->power_cost;
        }
    } else if(bio.id == "bio_magnet") {
        for (int i = posx - 10; i <= posx + 10; i++) {
            for (int j = posy - 10; j <= posy + 10; j++) {
                if (g->m.i_at(i, j).size() > 0) {
                    int t; //not sure why map:sees really needs this, but w/e
                    if (g->m.sees(i, j, posx, posy, -1, t)) {
                        traj = line_to(i, j, posx, posy, t);
                    } else {
                        traj = line_to(i, j, posx, posy, 0);
                    }
                }
                traj.insert(traj.begin(), point(i, j));
                for (unsigned k = 0; k < g->m.i_at(i, j).size(); k++) {
                    if (g->m.i_at(i, j)[k].made_of("iron") || g->m.i_at(i, j)[k].made_of("steel")) {
                        tmp_item = g->m.i_at(i, j)[k];
                        g->m.i_rem(i, j, k);
                        size_t l = 0;
                        for (l = 0; l < traj.size(); l++) {
                            int index = g->mon_at(traj[l].x, traj[l].y);
                            if (index != -1) {
                                if (g->zombie(index).hurt(tmp_item.weight() / 225)) {
                                    g->kill_mon(index, true);
                                }
                                g->m.add_item_or_charges(traj[l].x, traj[l].y, tmp_item);
                                l = traj.size() + 1;
                            } else if (l > 0 && g->m.move_cost(traj[l].x, traj[l].y) == 0) {
                                g->m.bash(traj[l].x, traj[l].y, tmp_item.weight() / 225, junk);
                                g->sound(traj[l].x, traj[l].y, 12, junk);
                                if (g->m.move_cost(traj[l].x, traj[l].y) == 0) {
                                    g->m.add_item_or_charges(traj[l - 1].x, traj[l - 1].y, tmp_item);
                                    l = traj.size() + 1;
                                }
                            }
                        }
                        if (l == traj.size()) {
                            g->m.add_item_or_charges(posx, posy, tmp_item);
                        }
                    }
                }
            }
        }
    } else if(bio.id == "bio_lockpick") {
        if(!g->choose_adjacent(_("Activate your bio lockpick where?"), dirx, diry)) {
            power_level += bionics["bio_lockpick"]->power_cost;
            return;
        }
        ter_id type = g->m.ter(dirx, diry);
        if (type  == t_door_locked || type == t_door_locked_alarm || type == t_door_locked_interior ) {
            moves -= 40;
            std::string door_name = rm_prefix(_("<door_name>door"));
            g->add_msg_if_player(this, _("With a satisfying click, the lock on the %s opens."),door_name.c_str());
            g->m.ter_set(dirx, diry, t_door_c);
        // Locked metal doors are the Lab and Bunker entries.  Those need to stay locked.
        } else if(type == t_door_bar_locked){
            moves -= 40;
            std::string door_name = rm_prefix(_("<door_name>door"));
            g->add_msg_if_player(this, _("The %s swings open..."), door_name.c_str()); //Could better copy the messages from lockpick....
            g->m.ter_set(dirx,diry, t_door_bar_o);
        } else if(type == t_chaingate_l){
            moves -=40;
            std::string gate_name = rm_prefix (_("<door_name>gate"));
            g->add_msg_if_player(this, _("With a satisfying click, the lock on the %s opens."), gate_name.c_str()); 
        } else if(type == t_door_c){
             g->add_msg(_("That door isn't locked."));
        } else {
            g->add_msg_if_player(this, _("You can't unlock that %s."), g->m.tername(dirx, diry).c_str());
        }
    } else if(bio.id == "bio_flashbang") {
        g->add_msg_if_player(this, _("You activate your integrated flashbang generator!"));
        g->flashbang(posx, posy, true);
    } else if(bio.id == "bio_shockwave") {
        g->shockwave(posx, posy, 3, 4, 2, 8, true);
        g->add_msg_if_player(this, _("You unleash a powerful shockwave!"));
    }
}

bool player::install_bionics(it_bionic *type)
{

    if (type == NULL) {
        debugmsg("Tried to install NULL bionic");
        return false;
    }
    if (bionics.count(type->id) == 0) {
        popup("invalid / unknown bionic id %s", type->id.c_str());
        return false;
    }
    if (has_bionic(type->id)) {
        if (!(type->id == "bio_power_storage" || type->id == "bio_power_storage_mkII")) {
            popup(_("You have already installed this bionic."));
            return false;
        }
    }

    int pl_skill = int_cur * 4 +
                   skillLevel("electronics") * 4 +
                   skillLevel("firstaid")    * 3 +
                   skillLevel("mechanics")   * 1;

    // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = float (pl_skill) - std::min( float (40),
                           float (pl_skill) - float (pl_skill) / float (10.0));

    // we will base chance_of_success on a ratio of skill and difficulty
    // when skill=difficulty, this gives us 1.  skill < difficulty gives a fraction.
    float skill_difficulty_parameter = float(adjusted_skill / (4.0 * type->difficulty));

    // when skill == difficulty, chance_of_success is 50%. Chance of success drops quickly below that
    // to reserve bionics for characters with the appropriate skill.  For more difficult bionics, the
    // curve flattens out just above 80%
    int chance_of_success = int((100 * skill_difficulty_parameter) /
                                (skill_difficulty_parameter + sqrt( 1 / skill_difficulty_parameter)));

    if (!query_yn(_("WARNING: %i percent chance of genetic damage, blood loss, or damage to existing bionics! Install anyway?"),
                  100 - chance_of_success)) {
        return false;
    }
    int pow_up = 0;
    if (type->id == "bio_power_storage" || type->id == "bio_power_storage_mkII") {
        pow_up = BATTERY_AMOUNT;
        if (type->id == "bio_power_storage_mkII") {
            pow_up = 10;
        }
    }

    practice(g->turn, "electronics", int((100 - chance_of_success) * 1.5));
    practice(g->turn, "firstaid", int((100 - chance_of_success) * 1.0));
    practice(g->turn, "mechanics", int((100 - chance_of_success) * 0.5));
    int success = chance_of_success - rng(1, 100);
    if (success > 0) {
        add_memorial_log(pgettext("memorial_male", "Installed bionic: %s."),
            pgettext("memorial_female", "Installed bionic: %s."),
            bionics[type->id]->name.c_str());
        if (pow_up) {
            max_power_level += pow_up;
            g->add_msg_if_player(this, _("Increased storage capacity by %i"), pow_up);
        } else {
            g->add_msg(_("Successfully installed %s."), bionics[type->id]->name.c_str());
            add_bionic(type->id);
        }
    } else {
        add_memorial_log(pgettext("memorial_male", "Installed bionic: %s."),
            pgettext("memorial_female", "Installed bionic: %s."),
            bionics[type->id]->name.c_str());
        bionics_install_failure(this, type, success);
    }
    g->refresh_all();
    return true;
}

void bionics_install_failure(player *u, it_bionic *type, int success)
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

    // for failure_level calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = float (pl_skill) - std::min( float (40),
                           float (pl_skill) - float (pl_skill) / float (10.0));

    // failure level is decided by how far off the character was from a successful install, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    int failure_level = int(sqrt(success * 4.0 * type->difficulty / float (adjusted_skill)));
    int fail_type = (failure_level > 5 ? 5 : failure_level);

    if (fail_type <= 0) {
        g->add_msg(_("The installation fails without incident."));
        return;
    }

    switch (rng(1, 5)) {
    case 1:
        g->add_msg(_("You flub the installation."));
        break;
    case 2:
        g->add_msg(_("You mess up the installation."));
        break;
    case 3:
        g->add_msg(_("The installation fails."));
        break;
    case 4:
        g->add_msg(_("The installation is a failure."));
        break;
    case 5:
        g->add_msg(_("You screw up the installation."));
        break;
    }

    if (fail_type == 3 && u->num_bionics() == 0) {
        fail_type = 2;    // If we have no bionics, take damage instead of losing some
    }

    switch (fail_type) {

    case 1:
        if (!(u->has_trait("NOPAIN"))) {
            g->add_msg(_("It really hurts!"));
            u->mod_pain( rng(failure_level * 3, failure_level * 6) );
        }
        break;

    case 2:
        g->add_msg(_("Your body is damaged!"));
        u->hurtall(rng(failure_level, failure_level * 2));
        break;

    case 3:
        if (u->num_bionics() <= failure_level) {
            g->add_msg(_("All of your existing bionics are lost!"));
        } else {
            g->add_msg(_("Some of your existing bionics are lost!"));
        }
        for (int i = 0; i < failure_level && u->remove_random_bionic(); i++) {
            ;
        }
        break;

    case 4:
        g->add_msg(_("You do damage to your genetics, causing mutation!"));
        while (failure_level > 0) {
            u->mutate();
            failure_level -= rng(1, failure_level + 2);
        }
        break;

    case 5: {
        g->add_msg(_("The installation is faulty!"));
        std::vector<bionic_id> valid;
        for (std::vector<std::string>::iterator it = faulty_bionics.begin() ; it != faulty_bionics.end(); ++it) {
            if (!u->has_bionic(*it)) {
                valid.push_back(*it);
            }
        }
        if (valid.empty()) { // We've got all the bad bionics!
            if (u->max_power_level > 0) {
                int old_power = u->max_power_level;
                g->add_msg(_("You lose power capacity!"));
                u->max_power_level = rng(0, u->max_power_level - 1);
                g->u.add_memorial_log(pgettext("memorial_male","Lost %d units of power capacity."),
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
    for (std::map<bionic_id, bionic_data*>::iterator bio = bionics.begin(); bio != bionics.end(); ++bio){
        delete bio->second;
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
    int cost = jsobj.get_int("cost", 0);
    int time = jsobj.get_int("time", 0);
    bool faulty = jsobj.get_bool("faulty", false);
    bool power_source = jsobj.get_bool("power_source", false);
    bool active = jsobj.get_bool("active", false);

    if (faulty) {
        faulty_bionics.push_back(id);
    }
    if (power_source) {
        power_source_bionics.push_back(id);
    }
    if (!active && id != "bio_null") {
        unpowered_bionics.push_back(id);
    }

    bionics[id] = new bionic_data(name, power_source, active, cost, time, description, faulty);
}
