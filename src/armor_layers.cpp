#include "player.h"
#include "armor_layers.h"
#include "catacharset.h"
#include "itype.h"
#include "input.h"
#include "output.h"

void player::sort_armor()
{
    it_armor *each_armor = 0;

    /* Define required height of the right pane:
    * + 3 - horizontal lines;
    * + 1 - caption line;
    * + 2 - innermost/outermost string lines;
    * + 8 - sub-categories (torso, head, eyes, etc.);
    * + 1 - gap;
    * number of lines required for displaying all items is calculated dynamically,
    * because some items can have multiple entries (i.e. cover a few parts of body).
    */

    int req_right_h = 3 + 1 + 2 + 8 + 1;
    for (int cover = 0; cover < num_bp; cover++) {
        for (size_t i = 0; i < worn.size(); ++i) {
            each_armor = dynamic_cast<it_armor *>(worn[i].type);
            if (each_armor->covers & mfb(cover)) {
                req_right_h++;
            }
        }
    }

    /* Define required height of the mid pane:
    * + 3 - horizontal lines;
    * + 1 - caption line;
    * + 8 - general properties
    * + 7 - ASSUMPTION: max possible number of flags @ item
    * + 9 - warmth & enc block
    */
    int req_mid_h = 3 + 1 + 8 + 7 + 9;

    const int win_h = std::min(TERMY, std::max(FULL_SCREEN_HEIGHT,
                                               std::max(req_right_h, req_mid_h)));
    const int win_w = FULL_SCREEN_WIDTH + (TERMX - FULL_SCREEN_WIDTH) / 3;
    const int win_x = TERMX / 2 - win_w / 2;
    const int win_y = TERMY / 2 - win_h / 2;

    int cont_h   = win_h - 4;
    int left_w   = 15;
    int width_sort  = (win_w - 4) / 2;
    int width_description = (win_w - 4) - left_w - width_sort;

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

    std::string  armor_cat[] = {_("Torso"), _("Head"), _("Eyes"), _("Mouth"), _("Arms"),
                                _("Hands"), _("Legs"), _("Feet"), _("All")
                               };

    // Layout window
    WINDOW *w_main = newwin(win_h, win_w, win_y, win_x);
    draw_border(w_main);
    mvwhline(w_main, 2, 1, 0, win_w - 2);
    mvwvline(w_main, 3, left_w + 1, 0, win_h - 4);
    mvwvline(w_main, 3, left_w + width_description + 2, 0, win_h - 4);
    // intersections
    mvwhline(w_main, 2, 0, LINE_XXXO, 1);
    mvwhline(w_main, 2, win_w - 1, LINE_XOXX, 1);
    mvwvline(w_main, 2, left_w + 1, LINE_OXXX, 1);
    mvwvline(w_main, win_h - 1, left_w + 1, LINE_XXOX, 1);
    mvwvline(w_main, 2, left_w + width_description + 2, LINE_OXXX, 1);
    mvwvline(w_main, win_h - 1, left_w + width_description + 2, LINE_XXOX, 1);
    wrefresh(w_main);

    // Subwindows (between lines)
    WINDOW *w_title       = newwin(1, win_w - 4, win_y + 1, win_x + 2);
    WINDOW *w_list        = newwin(cont_h, left_w,   win_y + 3, win_x + 1);
    WINDOW *w_description = newwin(cont_h, width_description, win_y + 3, win_x + left_w + 2);
    WINDOW *w_sort        = newwin(cont_h, width_sort,  win_y + 3, win_x + left_w + width_description + 3);

    input_context ctxt("SORT_ARMOR");
    ctxt.register_cardinal();
    ctxt.register_action("QUIT");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("MOVE_ARMOR");
    ctxt.register_action("ASSIGN_INVLETS");
    ctxt.register_action("HELP");
    ctxt.register_action("HELP_KEYBINDINGS");

    for (bool sorting = true; sorting; ) {
        werase(w_title);
        werase(w_list);
        werase(w_description);
        werase(w_sort);

        // top bar
        wprintz(w_title, c_white, _("Sort Armor"));
        wprintz(w_title, c_yellow, "  << %s >>", armor_cat[tabindex].c_str());
        tmp_str = string_format(_("Press %s for help"), ctxt.get_desc("HELP").c_str());
        mvwprintz(w_title, 0, win_w - utf8_width(tmp_str.c_str()) - 4,
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

        // Left list
        for (int drawindex = 0; drawindex < leftListSize; drawindex++) {
            int itemindex = leftListOffset + drawindex;
            each_armor = dynamic_cast<it_armor *>(tmp_worn[itemindex]->type);

            if (itemindex == leftListIndex) {
                mvwprintz(w_list, drawindex + 1, 0, c_yellow, ">>");
            }

            if (itemindex == selected) {
                mvwprintz(w_list, drawindex + 1, 3, dam_color[int(tmp_worn[itemindex]->damage + 1)],
                          each_armor->name.c_str());
            } else {
                mvwprintz(w_list, drawindex + 1, 2, dam_color[int(tmp_worn[itemindex]->damage + 1)],
                          each_armor->name.c_str());
            }
        }

        if (leftListSize > tmp_worn.size()) {
            mvwprintz(w_list, cont_h - 1, left_w - utf8_width(_("<more>")), c_ltblue, _("<more>"));
        }
        if (leftListSize == 0) {
            mvwprintz(w_list, cont_h - 1, left_w - utf8_width(_("<empty>")), c_ltblue, _("<empty>"));
        }

        // Items stats
        if (leftListSize) {
            draw_description_pane(w_description, tmp_worn[leftListIndex]);
        } else {
            mvwprintz(w_description, 0, 1, c_white, _("Nothing to see here!"));
        }

        // Right header
        mvwprintz(w_sort, 0, 0, c_ltgray, _("<Innermost>"));
        center_print(w_sort, 0, c_ltgray, _("(Warmth)"));
        mvwprintz(w_sort, 0, width_sort - utf8_width(_("Encumbrance")), c_ltgray, _("Encumbrance"));

        // Right list
        rightListSize = 0;
        for (int cover = 0, pos = 1; cover < num_bp; cover++) {
            if (rightListSize >= rightListOffset && pos <= cont_h - 2) {
                mvwprintz(w_sort, pos, 1, (cover == tabindex) ? c_yellow : c_white,
                          "%s:", armor_cat[ bp_order[cover] ].c_str());

                // Warmth
                if (bp_order[cover] != bp_eyes) {
                        center_print(w_sort, pos, bodytemp_color(bp_order[cover]),
                                     "(%d)", warmth(body_part(bp_order[cover])));
                }

                // Encumbrance
                int armorenc = 0;
                double layers = 0.0;
                int enc = encumb(body_part(bp_order[cover]), layers, armorenc);
                std::string enc_details = string_format("%d = %d + %d",
                                                        enc, enc - armorenc, armorenc);

                nc_color text_color = ( (enc > 0) ? c_white : c_ltgray);
                mvwprintz(w_sort, pos, width_sort - utf8_width(enc_details.c_str()) - 1,
                          text_color, enc_details.c_str());

                if (enc > 0) {
                    //Hack: overwrite the total encumbrance by colored duplicate
                    mvwprintz(w_sort, pos, width_sort - utf8_width(enc_details.c_str()) - 1,
                              c_yellow, "%d", enc);
                }

                pos++;
            }
            rightListSize++;
            for (size_t i = 0; i < worn.size(); ++i) {
                each_armor = dynamic_cast<it_armor *>(worn[i].type);
                if (each_armor->covers & mfb(bp_order[cover])) {
                    if (rightListSize >= rightListOffset && pos <= cont_h - 2) {
                        mvwprintz(w_sort, pos, 2 + worn[i].clothing_lvl(), dam_color[int(worn[i].damage + 1)],
                                  each_armor->name.c_str());

                        int storage_val = int(each_armor->storage);
                        if (storage_val > 0) {
                            wprintz(w_sort, c_ltgray, " [%d]", storage_val);
                        }
                        mvwprintz(w_sort, pos, width_sort - 2, c_ltgray, "%d",
                                  (worn[i].has_flag("FIT")) ? std::max(0, int(each_armor->encumber) - 1)
                                  : int(each_armor->encumber));
                        pos++;
                    }
                    rightListSize++;
                }
            }
        }

        // Right footer
        mvwprintz(w_sort, cont_h - 1, 0, c_ltgray, _("<Outermost>"));
        if (rightListSize > cont_h - 2) {
            mvwprintz(w_sort, cont_h - 1, width_sort - utf8_width(_("<more>")), c_ltblue, _("<more>"));
        }
        // F5
        wrefresh(w_title);
        wrefresh(w_list);
        wrefresh(w_description);
        wrefresh(w_sort);

        const std::string action = ctxt.handle_input();
        if (action == "UP" && leftListSize > 0) {
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
        } else if (action == "DOWN" && leftListSize > 0) {
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
        } else if (action == "LEFT") {
            tabindex--;
            if (tabindex < 0) {
                tabindex = tabcount - 1;
            }
            leftListIndex = leftListOffset = 0;
            selected = -1;
        } else if (action == "RIGHT") {
            tabindex = (tabindex + 1) % tabcount;
            leftListIndex = leftListOffset = 0;
            selected = -1;
        } else if (action == "NEXT_TAB") {
            rightListOffset++;
            if (rightListOffset + cont_h - 2 > rightListSize) {
                rightListOffset = rightListSize - cont_h + 2;
            }
        } else if (action == "PREV_TAB") {
            rightListOffset--;
            if (rightListOffset < 0) {
                rightListOffset = 0;
            }
        } else if (action == "MOVE_ARMOR") {
            if (selected >= 0) {
                selected = -1;
            } else {
                selected = leftListIndex;
            }
        } else if (action == "ASSIGN_INVLETS") {
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
        } else if (action == "HELP") {
            popup_getkey(_("\
Use the arrow- or keypad keys to navigate the left list.\n\
Press %s to select highlighted armor for reordering.\n\
Use %s / %s to scroll the right list.\n\
Press %s to assign special inventory letters to clothing.\n\
 \n\
[Encumbrance and Warmth] explanation:\n\
The first number is the encumbrance caused by the number of clothing on that body part.\n\
The second number is the summed encumbrance from all clothing on that body part.\n\
The sum of these values is the effective encumbrance value your character has for that body part."),
                ctxt.get_desc("MOVE_ARMOR").c_str(),
                ctxt.get_desc("PREV_TAB").c_str(),
                ctxt.get_desc("NEXT_TAB").c_str(),
                ctxt.get_desc("ASSIGN_INVLETS").c_str()
            );
            //TODO: refresh the window properly. Current method erases the intersection symbols
            draw_border(w_main); // hack to mark whole window for redrawing
            wrefresh(w_main);
        } else if (action == "QUIT") {
            sorting = false;
        }
    }

    delwin(w_title);
    delwin(w_list);
    delwin(w_description);
    delwin(w_sort);
    delwin(w_main);
}

void draw_description_pane(WINDOW *w_description, item *worn_item)
{
    it_armor *each_armor = dynamic_cast<it_armor *>(worn_item->type);
    int width_description = getmaxx(w_description);
    std::vector<std::string> props = clothing_properties(worn_item, width_description - 3);
    // Item name
    mvwprintz(w_description, 0, 1, dam_color[int(worn_item->damage + 1)], each_armor->name.c_str());
    // Layer level
    mvwprintz(w_description, 1, 2, c_white, "%s",
              worn_item->clothing_lvl_description(true).c_str());

    size_t i;
    for (i = 0; i < props.size(); ++i) {
        mvwprintz(w_description, i + 3, 2, c_ltgray, props[i].c_str());
    }
    i += 4;

    std::vector<std::string> desc = clothing_flags_description(worn_item);
    if (!desc.empty()) {
        for (size_t j = 0; j < desc.size(); ++j) {
            i += -1 + fold_and_print(w_description, i + j, 1, width_description,
                                     c_ltblue, "%s", desc[j].c_str());
        }
    }
}

std::vector<std::string> clothing_properties(item *worn_item, int width)
{
    it_armor *each_armor = dynamic_cast<it_armor *>(worn_item->type);
    std::vector<std::string> props;
    props.push_back(name_and_value(_("Coverage:"),
                    string_format("%3d", int(each_armor->coverage)), width));
    props.push_back(name_and_value(_("Encumbrance:"), string_format("%3d",
                    (worn_item->has_flag("FIT")) ? std::max(0, int(each_armor->encumber) - 1) :
                    int(each_armor->encumber)), width));
    props.push_back(name_and_value(_("Bash Protection:"),
                    string_format("%3d", int(worn_item->bash_resist())), width));
    props.push_back(name_and_value(_("Cut Protection:"),
                    string_format("%3d", int(worn_item->cut_resist())), width));
    props.push_back(name_and_value(_("Environmental Protection:"),
                    string_format("%3d", int(each_armor->env_resist)), width));
    props.push_back(name_and_value(_("Warmth:"),
                    string_format("%3d", int(each_armor->warmth)), width));
    props.push_back(name_and_value(_("Storage:"),
                    string_format("%3d", int(each_armor->storage)), width));
    return props;
}

std::vector<std::string> clothing_flags_description(item *worn_item)
{
    std::vector<std::string> description_stack;

    if (worn_item->has_flag("FIT")) {
        description_stack.push_back(_("It fits you well."));
    } else if (worn_item->has_flag("VARSIZE")) {
        description_stack.push_back(_("It could be refitted."));
    }

    if (worn_item->has_flag("HOOD")) {
        description_stack.push_back(_("It has a hood."));
    }
    if (worn_item->has_flag("POCKETS")) {
        description_stack.push_back(_("It has pockets."));
    }
    if (worn_item->has_flag("WATERPROOF")) {
        description_stack.push_back(_("It is waterproof."));
    }
    if (worn_item->has_flag("WATER_FRIENDLY")) {
        description_stack.push_back(_("It is water friendly."));
    }
    if (worn_item->has_flag("FANCY")) {
        description_stack.push_back(_("It looks fancy."));
    }
    if (worn_item->has_flag("SUPER_FANCY")) {
        description_stack.push_back(_("It looks really fancy."));
    }
    if (worn_item->has_flag("FLOATATION")) {
        description_stack.push_back(_("You will not drown today."));
    }
    if (worn_item->has_flag("OVERSIZE")) {
        description_stack.push_back(_("It is very bulky."));
    }
    if (worn_item->has_flag("SWIM_GOGGLES")) {
        description_stack.push_back(_("It helps you to see clearly underwater."));
    }
    return description_stack;
}
