#include "player.h"
#include "cursesdef.h"
#include "disease.h"
#include "input.h"
#include "output.h"

void player::sort_armor()
{
    const int win_h = FULL_SCREEN_HEIGHT + (TERMY - FULL_SCREEN_HEIGHT) / 3;
    const int win_w = FULL_SCREEN_WIDTH + (TERMX - FULL_SCREEN_WIDTH) / 3;
    const int win_x = TERMX / 2 - win_w / 2;
    const int win_y = TERMY / 2 - win_h / 2;

    int cont_h   = win_h - 4;
    int left_w   = (win_w - 4) / 3;
    int right_w  = left_w;
    int middle_w = (win_w - 4) - left_w - right_w;

    int tabindex = num_bp;
    int tabcount = num_bp + 1;

    int leftListSize;
    int leftListIndex  = 0;
    int leftListOffset = 0;
    int selected       = -1;

    int rightListSize;
    int rightListOffset = 0;

    item tmp_item;
    std::vector<item *> tmp_worn;
    std::string tmp_str;
    it_armor *each_armor = 0;

    std::string  armor_cat[] = {_("Torso"), _("Head"), _("Eyes"), _("Mouth"), _("Arms"),
                                _("Hands"), _("Legs"), _("Feet"), _("All")
                               };

    // Layout window
    WINDOW *w_sort_armor = newwin(win_h, win_w, win_y, win_x);
    draw_border(w_sort_armor);
    mvwhline(w_sort_armor, 2, 1, 0, win_w - 2);
    mvwvline(w_sort_armor, 3, left_w + 1, 0, win_h - 4);
    mvwvline(w_sort_armor, 3, left_w + middle_w + 2, 0, win_h - 4);
    // intersections
    mvwhline(w_sort_armor, 2, 0, LINE_XXXO, 1);
    mvwhline(w_sort_armor, 2, win_w - 1, LINE_XOXX, 1);
    mvwvline(w_sort_armor, 2, left_w + 1, LINE_OXXX, 1);
    mvwvline(w_sort_armor, win_h - 1, left_w + 1, LINE_XXOX, 1);
    mvwvline(w_sort_armor, 2, left_w + middle_w + 2, LINE_OXXX, 1);
    mvwvline(w_sort_armor, win_h - 1, left_w + middle_w + 2, LINE_XXOX, 1);
    wrefresh(w_sort_armor);

    // Subwindows (between lines)
    WINDOW *w_sort_cat    = newwin(1, win_w - 4, win_y + 1, win_x + 2);
    WINDOW *w_sort_left   = newwin(cont_h, left_w,   win_y + 3, win_x + 1);
    WINDOW *w_sort_middle = newwin(cont_h, middle_w, win_y + 3, win_x + left_w + 2);
    WINDOW *w_sort_right  = newwin(cont_h, right_w,  win_y + 3, win_x + left_w + middle_w + 3);

    nc_color dam_color[] = {c_green, c_ltgreen, c_yellow, c_magenta, c_ltred, c_red};

    for (bool sorting = true; sorting; ) {
        werase(w_sort_cat);
        werase(w_sort_left);
        werase(w_sort_middle);
        werase(w_sort_right);

        // top bar
        wprintz(w_sort_cat, c_white, _("Sort Armor"));
        wprintz(w_sort_cat, c_yellow, "  << %s >>", armor_cat[tabindex].c_str());
        tmp_str = _("Press '?' for help");
        mvwprintz(w_sort_cat, 0, win_w - utf8_width(tmp_str.c_str()) - 4,
                  c_white, tmp_str.c_str());

        // Create ptr list of items to display
        tmp_worn.clear();
        if (tabindex == 8) { // All
            for (size_t i = 0; i < worn.size(); ++i) {
                tmp_worn.push_back(&worn[i]);
            }
        } else { // bp_*
            for (size_t i = 0; i < worn.size(); ++i) {
                each_armor = dynamic_cast<it_armor *>(worn[i].type);
                if (each_armor->covers & mfb(tabindex)) {
                    tmp_worn.push_back(&worn[i]);
                }
            }
        }
        leftListSize = (tmp_worn.size() < cont_h - 2) ? tmp_worn.size() : cont_h - 2;

        // Left header
        mvwprintz(w_sort_left, 0, 0, c_ltgray, _("(Innermost)"));
        mvwprintz(w_sort_left, 0, left_w - utf8_width(_("Storage")), c_ltgray, _("Storage"));

        // Left list
        for (int drawindex = 0; drawindex < leftListSize; drawindex++) {
            int itemindex = leftListOffset + drawindex;
            each_armor = dynamic_cast<it_armor *>(tmp_worn[itemindex]->type);

            if (itemindex == leftListIndex) {
                mvwprintz(w_sort_left, drawindex + 1, 0, c_yellow, ">>");
            }

            if (itemindex == selected) {
                mvwprintz(w_sort_left, drawindex + 1, 3, dam_color[int(tmp_worn[itemindex]->damage + 1)],
                          each_armor->name.c_str());
            } else {
                mvwprintz(w_sort_left, drawindex + 1, 2, dam_color[int(tmp_worn[itemindex]->damage + 1)],
                          each_armor->name.c_str());
            }
            mvwprintz(w_sort_left, drawindex + 1, left_w - 3, c_ltgray, "%3d", int(each_armor->storage));
        }

        // Left footer
        mvwprintz(w_sort_left, cont_h - 1, 0, c_ltgray, _("(Outermost)"));
        if (leftListSize > tmp_worn.size()) {
            mvwprintz(w_sort_left, cont_h - 1, left_w - utf8_width(_("<more>")), c_ltblue, _("<more>"));
        }
        if (leftListSize == 0) {
            mvwprintz(w_sort_left, cont_h - 1, left_w - utf8_width(_("<empty>")), c_ltblue, _("<empty>"));
        }

        // Items stats
        if (leftListSize) {
            each_armor = dynamic_cast<it_armor *>(tmp_worn[leftListIndex]->type);

            mvwprintz(w_sort_middle, 0, 1, c_white, each_armor->name.c_str());
            mvwprintz(w_sort_middle, 1, 2, c_ltgray, _("Coverage: "));
            mvwprintz(w_sort_middle, 2, 2, c_ltgray, _("Encumbrance: "));
            mvwprintz(w_sort_middle, 3, 2, c_ltgray, _("Bash protection: "));
            mvwprintz(w_sort_middle, 4, 2, c_ltgray, _("Cut protection: "));
            mvwprintz(w_sort_middle, 5, 2, c_ltgray, _("Warmth: "));
            mvwprintz(w_sort_middle, 6, 2, c_ltgray, _("Storage: "));

            mvwprintz(w_sort_middle, 1, middle_w - 4, c_ltgray, "%d", int(each_armor->coverage));
            mvwprintz(w_sort_middle, 2, middle_w - 4, c_ltgray, "%d",
                      (tmp_worn[leftListIndex]->has_flag("FIT")) ?
                      std::max(0, int(each_armor->encumber) - 1) : int(each_armor->encumber));
            mvwprintz(w_sort_middle, 3, middle_w - 4, c_ltgray, "%d",
                      int(tmp_worn[leftListIndex]->bash_resist()));
            mvwprintz(w_sort_middle, 4, middle_w - 4, c_ltgray, "%d",
                      int(tmp_worn[leftListIndex]->cut_resist()));
            mvwprintz(w_sort_middle, 5, middle_w - 4, c_ltgray, "%d", int(each_armor->warmth));
            mvwprintz(w_sort_middle, 6, middle_w - 4, c_ltgray, "%d", int(each_armor->storage));

            // Item flags
            if (tmp_worn[leftListIndex]->has_flag("FIT")) {
                tmp_str = _("It fits you well.\n");
            } else if (tmp_worn[leftListIndex]->has_flag("VARSIZE")) {
                tmp_str = _("It could be refitted.\n");
            } else {
                tmp_str = "";
            }

            if (tmp_worn[leftListIndex]->has_flag("SKINTIGHT")) {
                tmp_str += _("It lies close to the skin.\n");
            }
            if (tmp_worn[leftListIndex]->has_flag("POCKETS")) {
                tmp_str += _("It has pockets.\n");
            }
            if (tmp_worn[leftListIndex]->has_flag("HOOD")) {
                tmp_str += _("It has a hood.\n");
            }
            if (tmp_worn[leftListIndex]->has_flag("WATERPROOF")) {
                tmp_str += _("It is waterproof.\n");
            }
            if (tmp_worn[leftListIndex]->has_flag("WATER_FRIENDLY")) {
                tmp_str += _("It is water friendly.\n");
            }
            if (tmp_worn[leftListIndex]->has_flag("FANCY")) {
                tmp_str += _("It looks fancy.\n");
            }
            if (tmp_worn[leftListIndex]->has_flag("SUPER_FANCY")) {
                tmp_str += _("It looks really fancy.\n");
            }
            if (tmp_worn[leftListIndex]->has_flag("FLOATATION")) {
                tmp_str += _("You will not drown today.\n");
            }
            if (tmp_worn[leftListIndex]->has_flag("OVERSIZE")) {
                tmp_str += _("It is very bulky.\n");
            }
            if (tmp_worn[leftListIndex]->has_flag("SWIM_GOGGLES")) {
                tmp_str += _("It helps you to see clearly underwater.\n");
            }
            //WATCH
            //ALARMCLOCK

            mvwprintz(w_sort_middle, 8, 0, c_ltblue, tmp_str.c_str());
        } else {
            mvwprintz(w_sort_middle, 0, 1, c_white, _("Nothing to see here!"));
        }

        // Player encumbrance - altered copy of '@' screen
        mvwprintz(w_sort_middle, cont_h - 9, 1, c_white, _("Encumbrance and Warmth"));
        for (int i = 0; i < num_bp; ++i) {
            int enc, armorenc;
            double layers;
            layers = armorenc = 0;
            enc = encumb(body_part(i), layers, armorenc);
            if (leftListSize && (each_armor->covers & mfb(i))) {
                mvwprintz(w_sort_middle, cont_h - 8 + i, 2, c_green, "%s:", armor_cat[i].c_str());
            } else {
                mvwprintz(w_sort_middle, cont_h - 8 + i, 2, c_ltgray, "%s:", armor_cat[i].c_str());
            }
            mvwprintz(w_sort_middle, cont_h - 8 + i, middle_w - 16, c_ltgray, "%d+%d = ", armorenc,
                      enc - armorenc);
            wprintz(w_sort_middle, encumb_color(enc), "%d" , enc);

            nc_color color = c_ltgray;
            if (i == bp_eyes) {
                continue;    // Eyes don't count towards warmth
            } else if (temp_conv[i] >  BODYTEMP_SCORCHING) {
                color = c_red;
            } else if (temp_conv[i] >  BODYTEMP_VERY_HOT) {
                color = c_ltred;
            } else if (temp_conv[i] >  BODYTEMP_HOT) {
                color = c_yellow;
            } else if (temp_conv[i] >  BODYTEMP_COLD) {
                color = c_green;
            } else if (temp_conv[i] >  BODYTEMP_VERY_COLD) {
                color = c_ltblue;
            } else if (temp_conv[i] >  BODYTEMP_FREEZING) {
                color = c_cyan;
            } else if (temp_conv[i] <= BODYTEMP_FREEZING) {
                color = c_blue;
            }
            mvwprintz(w_sort_middle, cont_h - 8 + i, middle_w - 6, color, "(%3d)", warmth(body_part(i)));
        }

        // Right header
        mvwprintz(w_sort_right, 0, 0, c_ltgray, _("(Innermost)"));
        mvwprintz(w_sort_right, 0, right_w - utf8_width(_("Encumbrance")), c_ltgray, _("Encumbrance"));

        // Right list
        rightListSize = 0;
        for (int cover = 0, pos = 1; cover < num_bp; cover++) {
            if (rightListSize >= rightListOffset && pos <= cont_h - 2) {
                if (cover == tabindex) {
                    mvwprintz(w_sort_right, pos, 1, c_yellow, "%s:", armor_cat[cover].c_str());
                } else {
                    mvwprintz(w_sort_right, pos, 1, c_white, "%s:", armor_cat[cover].c_str());
                }
                pos++;
            }
            rightListSize++;
            for (size_t i = 0; i < worn.size(); ++i) {
                each_armor = dynamic_cast<it_armor *>(worn[i].type);
                if (each_armor->covers & mfb(cover)) {
                    if (rightListSize >= rightListOffset && pos <= cont_h - 2) {
                        mvwprintz(w_sort_right, pos, 2, dam_color[int(worn[i].damage + 1)],
                                  each_armor->name.c_str());
                        mvwprintz(w_sort_right, pos, right_w - 2, c_ltgray, "%d",
                                  (worn[i].has_flag("FIT")) ? std::max(0, int(each_armor->encumber) - 1)
                                  : int(each_armor->encumber));
                        pos++;
                    }
                    rightListSize++;
                }
            }
        }

        // Right footer
        mvwprintz(w_sort_right, cont_h - 1, 0, c_ltgray, _("(Outermost)"));
        if (rightListSize > cont_h - 2) {
            mvwprintz(w_sort_right, cont_h - 1, right_w - utf8_width(_("<more>")), c_ltblue, _("<more>"));
        }
        // F5
        wrefresh(w_sort_cat);
        wrefresh(w_sort_left);
        wrefresh(w_sort_middle);
        wrefresh(w_sort_right);

        switch (input()) {
        case '8':
        case 'k':
            if (!leftListSize) {
                break;
            }
            leftListIndex--;
            if (leftListIndex < 0) {
                leftListIndex = tmp_worn.size() - 1;
            }

            // Scrolling logic
            leftListOffset = (leftListIndex < leftListOffset) ? leftListIndex : leftListOffset;
            if (!((leftListIndex >= leftListOffset) && (leftListIndex < leftListOffset + leftListSize))) {
                leftListOffset = leftListIndex - leftListSize + 1;
                leftListOffset = (leftListOffset > 0) ? leftListOffset : 0;
            }

            // move selected item
            if (selected >= 0) {
                tmp_item = *tmp_worn[leftListIndex];
                for (size_t i = 0; i < worn.size(); ++i)
                    if (&worn[i] == tmp_worn[leftListIndex]) {
                        worn[i] = *tmp_worn[selected];
                    }

                for (size_t i = 0; i < worn.size(); ++i)
                    if (&worn[i] == tmp_worn[selected]) {
                        worn[i] = tmp_item;
                    }

                selected = leftListIndex;
            }
            break;

        case '2':
        case 'j':
            if (!leftListSize) {
                break;
            }
            leftListIndex = (leftListIndex + 1) % tmp_worn.size();

            // Scrolling logic
            if (!((leftListIndex >= leftListOffset) && (leftListIndex < leftListOffset + leftListSize))) {
                leftListOffset = leftListIndex - leftListSize + 1;
                leftListOffset = (leftListOffset > 0) ? leftListOffset : 0;
            }

            // move selected item
            if (selected >= 0) {
                tmp_item = *tmp_worn[leftListIndex];
                for (size_t i = 0; i < worn.size(); ++i)
                    if (&worn[i] == tmp_worn[leftListIndex]) {
                        worn[i] = *tmp_worn[selected];
                    }

                for (size_t i = 0; i < worn.size(); ++i)
                    if (&worn[i] == tmp_worn[selected]) {
                        worn[i] = tmp_item;
                    }

                selected = leftListIndex;
            }
            break;

        case '4':
        case 'h':
            tabindex--;
            if (tabindex < 0) {
                tabindex = tabcount - 1;
            }
            leftListIndex = leftListOffset = 0;
            selected = -1;
            break;

        case '\t':
        case '6':
        case 'l':
            tabindex = (tabindex + 1) % tabcount;
            leftListIndex = leftListOffset = 0;
            selected = -1;
            break;

        case '>':
            rightListOffset++;
            if (rightListOffset + cont_h - 2 > rightListSize) {
                rightListOffset = rightListSize - cont_h + 2;
            }
            break;

        case '<':
            rightListOffset--;
            if (rightListOffset < 0) {
                rightListOffset = 0;
            }
            break;

        case 's':
            if (selected >= 0) {
                selected = -1;
            } else {
                selected = leftListIndex;
            }
            break;

        case 'r': {
            // Start with last armor (the most unimportant one?)
            int worn_index = worn.size() - 1;
            int invlet_index = inv_chars.size() - 1;
            while (invlet_index >= 0 && worn_index >= 0) {
                const char invlet = inv_chars[invlet_index];
                item &w = worn[worn_index];
                if (invlet == w.invlet) {
                    worn_index--;
                } else if (has_item(invlet)) {
                    invlet_index--;
                } else {
                    w.invlet = invlet;
                    worn_index--;
                    invlet_index--;
                }
            }
        }
        break;

        case '?': {
            popup_getkey(_("\
Use the arrow- or keypad keys to navigate the left list.\n\
Press 's' to select highlighted armor for reordering.\n\
Use PageUp/PageDown to scroll the right list.\n\
Press 'r' to assign special inventory letters to clothing.\n\
 \n\
[Encumbrance and Warmth] explanation:\n\
The first number is the summed encumbrance from all clothing on that bodypart.\n\
The second number is the encumbrance caused by the number of clothing on that bodypart.\n\
The sum of these values is the effective encumbrance value your character has for that bodypart."));
            draw_border(w_sort_armor); // hack to mark whole window for redrawing
            wrefresh(w_sort_armor);
            break;
        }

        case KEY_ESCAPE:
        case 'q':
            sorting = false;
            break;
        }
    }

    delwin(w_sort_cat);
    delwin(w_sort_left);
    delwin(w_sort_middle);
    delwin(w_sort_right);
    delwin(w_sort_armor);
}
