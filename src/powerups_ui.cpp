#include "player.h"
#include "game.h"
#include "rng.h"
#include "input.h"
#include "item.h"
#include "bionics.h"
#include "powerups_ui.h"
#include "line.h"
#include "json.h"
#include "messages.h"
#include "item_factory.h"
#include <math.h>    //sqrt
#include <algorithm> //std::min
#include <sstream>

void draw_powerups_titlebar(WINDOW *window, player *p, std::string menu_mode)
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

void player::draw_powerups_window()
{
    std::vector <bionic *> passive;
    std::vector <bionic *> active;
    for (std::vector<bionic>::iterator it = my_bionics.begin();
         it != my_bionics.end(); ++it) {
        if (!bionics[it->id]->activated) {
            passive.push_back(&*it);
        } else {
            active.push_back(&*it);
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
                    mvwprintz(wBio, list_start_y + i, second_column + 2, type,
                              (active[i]->powered ? _("%s - ON") : _("%s - %d PU / %d turns")),
                              bionics[active[i]->id]->name.c_str(),
                              bionics[active[i]->id]->power_cost,
                              bionics[active[i]->id]->charge_time);
                }
            }

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
        draw_powerups_titlebar(w_title, this, menu_mode);
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
                    itype_id weapon_id = weapon.type->id;
                    int b = tmp - &my_bionics[0];
                    if (tmp->powered) {
                        tmp->powered = false;
                        add_msg(m_neutral, _("%s powered off."), bio_data.name.c_str());

                        deactivate_bionic(b);
                    } else if (power_level >= bio_data.power_cost ||
                               (weapon_id == "bio_claws_weapon" && bio_id == "bio_claws_weapon") ||
                               (weapon_id == "bio_blade_weapon" && bio_id == "bio_blade_weapon")) {

                        // this will clear the bionics menu for targeting purposes
                        wBioptr.reset();
                        w_titleptr.reset();
                        w_descriptionptr.reset();
                        g->draw();

                        /*
                        Activate selected unit and, if there is no dedicated message for it,
                        print a generic message about activation
                        */
                        if (!activate_bionic(b)) {
                                add_msg(m_neutral, _("You activate your %s."), bio_data.name.c_str());
                        }

                        if (bio_id == "bio_cqb") {
                            pick_style();
                        }

                    } else {
                        popup( _( "You don't have enough power to activate the %s." ), bio_data.name.c_str() );
                        redraw = true;
                        continue;
                    }
                    // Action done, leave screen
                    return;
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

