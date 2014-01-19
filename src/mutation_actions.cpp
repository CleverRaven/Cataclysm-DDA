#include "player.h"
#include "game.h"
#include "rng.h"
#include "input.h"
#include "keypress.h"
#include "item.h"
#include "mutation.h"
#include "mutation_actions.h"
#include "line.h"
#include "json.h"

std::map<std::string, mut_action_data *> mut_actions;

mut_action_data::mut_action_data(std::string new_name, bool new_activated,
                                 std::string new_description) : description(new_description)
{
    name = new_name;
    activated = new_activated;
}

void show_action_titlebar(WINDOW *window, player *p, bool activating, bool reassigning)
{
    werase(window);

    std::string caption = _("BEAST SENSES -");
    int cap_offset = utf8_width(caption.c_str()) + 2;
    mvwprintz(window, 0, 1, c_green, caption.c_str());

    std::string desc;
    int desc_length = getmaxx(window) - cap_offset;

    if(reassigning) {
        desc = _("Reassigning.\nSelect an action to reassign or press SPACE to cancel.");
    } else if(activating) {
        desc = _("Activating. Press <color_yellow>!</color> to examine your features.\
\nPress <color_yellow>=</color> to reassign a key.");
    } else {
        desc = _("Examining. Press <color_yellow>!</color> to activate your features.\
\nPress <color_yellow>=</color> to reassign a key.");
    }
    fold_and_print(window, 0, cap_offset, desc_length, c_white, desc.c_str());

    wrefresh(window);
}

void player::choose_action()
{
    int HEIGHT = TERMY;
    int WIDTH = FULL_SCREEN_WIDTH;
    int START_X = (TERMX - WIDTH) / 2;
    int START_Y = (TERMY - HEIGHT) / 2;
    WINDOW *wAct = newwin(HEIGHT, WIDTH, START_Y, START_X);
    int DESCRIPTION_WIDTH = WIDTH - 2; // -2 for the borders
    int DESCRIPTION_HEIGHT = 5;
    int DESCRIPTION_START_X = getbegx(wAct) + 1; // +1 to avoid border
    int DESCRIPTION_START_Y = getmaxy(wAct) - DESCRIPTION_HEIGHT - 1; // At the bottom of the window, -1 to avoid border
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

    std::vector <mut_action *> action;
    std::vector <mut_action *> state;

    for (size_t i = 0; i < my_mut_actions.size(); i++) {
        if (!mut_actions[my_mut_actions[i].id]->activated) {
            action.push_back(&my_mut_actions[i]);
        } else {
            state.push_back(&my_mut_actions[i]);
        }
    }

    int HEADER_LINE_Y = TITLE_START_Y + TITLE_HEIGHT; // + lines with text in titlebar
    int DESCRIPTION_LINE_Y = DESCRIPTION_START_Y - 1;

    const int list_height = DESCRIPTION_LINE_Y - HEADER_LINE_Y - 2;
    int max_scroll_position = mut_actions.size() - list_height;
    int first_column = 2;

    while(true) {
        const int list_start_y = HEADER_LINE_Y + 2 - scroll_position;
        if(redraw) {
            redraw = false;

            werase(wAct);
            draw_border(wAct);
            nc_color type;

            draw_exam_window(wAct, DESCRIPTION_LINE_Y, (!reassigning && !activating));
            for (int i = 1; i < WIDTH - 1; i++) {
                mvwputch(wAct, HEADER_LINE_Y, i, BORDER_COLOR, LINE_OXOX); // Draw line under title
            }

            // Draw symbols to connect additional lines to border
            mvwputch(wAct, HEADER_LINE_Y, 0, BORDER_COLOR, LINE_XXXO); // |-
            mvwputch(wAct, HEADER_LINE_Y, WIDTH - 1, BORDER_COLOR, LINE_XOXX); // -|

            mvwprintz(wAct, HEADER_LINE_Y + 1, first_column, c_ltblue, _("Actions:"));

            if (action.size() <= 0) {
                mvwprintz(wAct, list_start_y, first_column, c_ltgray, _("N/A"));
            } else {
                for (size_t i = scroll_position; i < mut_actions.size(); i++) {
                    if (list_start_y + i == ((!activating && !reassigning) ?
                                             DESCRIPTION_LINE_Y : HEIGHT - 1)) {
                        break;
                    }
                type = c_cyan;
                mvwputch(wAct, list_start_y + i, first_column, type, action[i]->invlet);
                mvwprintz(wAct, list_start_y + i, first_column + 2, type,
                          mut_actions[action[i]->id]->name.c_str());
                }
            }
            if(scroll_position > 0) {
                mvwputch(wAct, HEADER_LINE_Y + 2, 0, c_ltgreen, '^');
            }
            if(scroll_position < max_scroll_position && max_scroll_position > 0) {
                mvwputch(wAct, (!activating && !reassigning) ? ((HEADER_LINE_Y + 2) +
                         list_height - 1) : (HEIGHT - 2), 0, c_ltgreen, 'v');
            }
        }
        wrefresh(wAct);
        show_action_titlebar(w_title, this, activating, reassigning);
        long ch = getch();
        mut_action *tmp = NULL;
        if (reassigning) {
            reassigning = false;
            tmp = mut_action_by_invlet(ch);
            if (tmp == 0) {
                // Selected an non-existing actions (or escape, or ...)
                continue;
            }
            redraw = true;
            const char newch = popup_getkey(_("%s; enter new letter."),
                                            mut_actions[tmp->id]->name.c_str());
            wrefresh(wAct);
            if(newch == ch || newch == ' ' || newch == KEY_ESCAPE) {
                continue;
            }
            mut_action *otmp = mut_action_by_invlet(newch);
            // if there is already a bionic with the new invlet, the invlet
            // is considered valid.
            if(otmp == 0 && inv_chars.find(newch) == std::string::npos) {
                // TODO separate list of letters for actions
                popup(_("%c is not a valid letter."), newch);
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
            draw_exam_window(wAct, DESCRIPTION_LINE_Y, (!reassigning && !activating));
            redraw = true;
        } else {
            tmp = mut_action_by_invlet(ch);
            if(tmp == 0) {
                // entered a key that is not mapped to any action,
                // -> leave screen
                break;
            }
            const std::string &mut_act_id = tmp->id;
            const mut_action_data &data = *mut_actions[mut_act_id];
            if (activating) {
                if (data.activated) {
                    if (tmp->activated) {
                        tmp->activated = false;
                        g->add_msg(_("You stop using your %s."), data.name.c_str());
                    } else {
                        use_feature(mut_act_id);
                    }
                    // Action done, leave screen
                    break;
                } else {
                    popup(_("\
You can not activate %s!  To read a description of \
%s, press '!', then '%c'."), data.name.c_str(), data.name.c_str(), tmp->invlet);
                    redraw = true;
                }
            } else { // Describing bionics, not activating them!
                draw_exam_window(wAct, DESCRIPTION_LINE_Y, true);
                // Clear the lines first
                werase(w_description);
                fold_and_print(w_description, 0, 0, WIDTH - 2, c_ltblue, data.description.c_str());
                wrefresh(w_description);
            }
        }
    }
    werase(wAct);
    wrefresh(wAct);
    delwin(w_title);
    delwin(w_description);
    delwin(wAct);
    erase();
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

void player::use_feature(std::string id)
{
    if (id == "shout") {
        //shout
    } else {
        //bug
    }
}

void load_mut_action(JsonObject &jsobj)
{
    std::string id = jsobj.get_string("id");
    std::string name = _(jsobj.get_string("name").c_str());
    std::string description = _(jsobj.get_string("description").c_str());
    bool active = jsobj.get_bool("active", false);

    mut_actions[id] = new mut_action_data(name, active, description);
}
