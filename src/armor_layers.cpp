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

    const int win_h = 24; // temp
                      /*std::min(TERMY, std::max(FULL_SCREEN_HEIGHT,
                                               std::max(req_right_h, req_mid_h)));*/
    const int win_w = FULL_SCREEN_WIDTH + (TERMX - FULL_SCREEN_WIDTH) / 3;
    const int win_x = TERMX / 2 - win_w / 2;
    const int win_y = TERMY / 2 - win_h / 2;

    int content_h = win_h - 4;
    int list_w = 8;
    int sort_w = (win_w - 3) * 3 / 5;
    int desc_w = (win_w - 3) - list_w - sort_w;

    int tabindex = num_bp;
    int tabcount = num_bp + 1;

    int list_list_size;
    int list_list_index  = 0;
    int list_list_offset = 0;
    int selected         = -1;

    int sort_list_size;
    int sort_list_offset = 0;

    item tmp_item;
    std::vector<item *> tmp_worn;
    std::string tmp_str;

    std::string  armor_cat[] = {_("Torso"), _("Head"), _("Eyes"), _("Mouth"), _("Arms"),
                                _("Hands"), _("Legs"), _("Feet"), _("All")
                               };

    // Layout window
    WINDOW *w_main = newwin(win_h, win_w, win_y, win_x);
    draw_background(w_main, sort_w, desc_w);

    // Subwindows (between lines)
    WINDOW *w_title = newwin(1, win_w - 4, win_y + 1, win_x + 2);
    WINDOW *w_sort  = newwin(content_h - 2, sort_w, win_y + 4, win_x);
    WINDOW *w_desc  = newwin(content_h, desc_w, win_y + 3, win_x + sort_w + 1);
    WINDOW *w_list  = newwin(content_h, list_w, win_y + 3, win_x + sort_w + desc_w + 2);

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
        werase(w_desc);
        werase(w_sort);

        // Top bar
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
        list_list_size = (tmp_worn.size() < content_h - 2) ? tmp_worn.size() : content_h - 2;

        // Left list
        for (int drawindex = 0; drawindex < list_list_size; drawindex++) {
            int itemindex = list_list_offset + drawindex;
            each_armor = dynamic_cast<it_armor *>(tmp_worn[itemindex]->type);

            if (itemindex == list_list_index) {
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

        if (list_list_size > tmp_worn.size()) {
            mvwprintz(w_list, content_h - 1, list_w - utf8_width(_("<more>")), c_ltblue, _("<more>"));
        }
        if (list_list_size == 0) {
            mvwprintz(w_list, content_h - 1, list_w - utf8_width(_("<empty>")), c_ltblue, _("<empty>"));
        }

        // Items stats
        if (list_list_size) {
            draw_description_pane(w_desc, tmp_worn[list_list_index]);
        } else {
            mvwprintz(w_desc, 0, 1, c_white, _("Nothing to see here!"));
        }

        // Right list
        sort_list_size = 0;
        for (int cover = 0, pos = 0; cover < num_bp; cover++) {
            if (sort_list_size >= sort_list_offset && pos <= content_h - 2) {
                mvwprintz(w_sort, pos, 2, (cover == tabindex) ? c_yellow : c_white,
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
                mvwprintz(w_sort, pos, sort_w - utf8_width(enc_details.c_str()) - 1,
                          text_color, enc_details.c_str());

                if (enc > 0) {
                    //Hack: overwrite the total encumbrance by colored duplicate
                    mvwprintz(w_sort, pos, sort_w - utf8_width(enc_details.c_str()) - 1,
                              c_yellow, "%d", enc);
                }

                pos++;
            }
            sort_list_size++;
            for (size_t i = 0; i < worn.size(); ++i) {
                each_armor = dynamic_cast<it_armor *>(worn[i].type);
                if (each_armor->covers & mfb(bp_order[cover])) {
                    if (sort_list_size >= sort_list_offset && pos <= content_h - 2) {
                        int lvl = worn[i].clothing_lvl() + 2;

                        // Debug only
                        if (true) {
                            mvwhline(w_sort, pos, 1, '>', lvl, c_yellow);
                        }

                        mvwprintz(w_sort, pos, lvl + 1, dam_color[int(worn[i].damage + 1)],
                                  each_armor->name.c_str());

                        // Storage
                        int storage_val = int(each_armor->storage);
                        if (storage_val > 0) {
                            wprintz(w_sort, c_ltgray, " [%d]", storage_val);
                        }
                        mvwprintz(w_sort, pos, sort_w - 2, c_ltgray, "%d",
                                  (worn[i].has_flag("FIT")) ? std::max(0, int(each_armor->encumber) - 1)
                                  : int(each_armor->encumber));
                        pos++;
                    }
                    sort_list_size++;
                }
            }
        }
        draw_scrollbar(w_sort, list_list_index, content_h - 2, sort_list_size, 0, 0, BORDER_COLOR);
        // F5
        wrefresh(w_title);
        wrefresh(w_list);
        wrefresh(w_desc);
        wrefresh(w_sort);

        const std::string action = ctxt.handle_input();
        if (action == "UP" && list_list_size > 0) {
            list_list_index--;
            if (list_list_index < 0) {
                list_list_index = tmp_worn.size() - 1;
            }

            // Scrolling logic
            list_list_offset = (list_list_index < list_list_offset) ? list_list_index : list_list_offset;
            if (!((list_list_index >= list_list_offset) && (list_list_index < list_list_offset + list_list_size))) {
                list_list_offset = list_list_index - list_list_size + 1;
                list_list_offset = (list_list_offset > 0) ? list_list_offset : 0;
            }

            // move selected item
            if (selected >= 0) {
                tmp_item = *tmp_worn[list_list_index];
                for (size_t i = 0; i < worn.size(); ++i)
                    if (&worn[i] == tmp_worn[list_list_index]) {
                        worn[i] = *tmp_worn[selected];
                    }

                for (size_t i = 0; i < worn.size(); ++i)
                    if (&worn[i] == tmp_worn[selected]) {
                        worn[i] = tmp_item;
                    }

                selected = list_list_index;
            }
        } else if (action == "DOWN" && list_list_size > 0) {
            list_list_index = (list_list_index + 1) % tmp_worn.size();

            // Scrolling logic
            if (!((list_list_index >= list_list_offset) &&
                  (list_list_index < list_list_offset + list_list_size))) {
                list_list_offset = list_list_index - list_list_size + 1;
                list_list_offset = (list_list_offset > 0) ? list_list_offset : 0;
            }

            // move selected item
            if (selected >= 0) {
                tmp_item = *tmp_worn[list_list_index];
                for (size_t i = 0; i < worn.size(); ++i) {
                    if (&worn[i] == tmp_worn[list_list_index]) {
                        worn[i] = *tmp_worn[selected];
                    }
                }

                for (size_t i = 0; i < worn.size(); ++i) {
                    if (&worn[i] == tmp_worn[selected]) {
                        worn[i] = tmp_item;
                    }
                }

                selected = list_list_index;
            }
        } else if (action == "LEFT") {
            tabindex--;
            if (tabindex < 0) {
                tabindex = tabcount - 1;
            }
            list_list_index = list_list_offset = 0;
            selected = -1;
        } else if (action == "RIGHT") {
            tabindex = (tabindex + 1) % tabcount;
            list_list_index = list_list_offset = 0;
            selected = -1;
        } else if (action == "NEXT_TAB") {
            sort_list_offset++;
            if (sort_list_offset + content_h - 2 > sort_list_size) {
                sort_list_offset = sort_list_size - content_h + 2;
            }
        } else if (action == "PREV_TAB") {
            sort_list_offset--;
            if (sort_list_offset < 0) {
                sort_list_offset = 0;
            }
        } else if (action == "MOVE_ARMOR") {
            if (selected >= 0) {
                selected = -1;
            } else {
                selected = list_list_index;
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
[Encumbrance] explanation:\n\
The first number is the encumbrance caused by the number of clothing on that body part.\n\
The second number is the summed encumbrance from all clothing on that body part.\n\
The sum of these values is the effective encumbrance value your character has for that body part."),
                ctxt.get_desc("MOVE_ARMOR").c_str(),
                ctxt.get_desc("PREV_TAB").c_str(),
                ctxt.get_desc("NEXT_TAB").c_str(),
                ctxt.get_desc("ASSIGN_INVLETS").c_str()
            );
            // Refresh main window
            draw_background(w_main, sort_w, desc_w);
        } else if (action == "QUIT") {
            sorting = false;
        }
    }

    delwin(w_title);
    delwin(w_list);
    delwin(w_desc);
    delwin(w_sort);
    delwin(w_main);
}

void draw_background(WINDOW *w, int sort_w, int desc_w)
{
    int win_w = getmaxx(w);
    int win_h = getmaxy(w);

    draw_border(w);

    // Horizontal line
    mvwhline(w, 2, 1, 0, win_w - 2);
    mvwputch(w, 2, 0, BORDER_COLOR, LINE_XXXO);
    mvwputch(w, 2, win_w - 1, BORDER_COLOR, LINE_XOXX);

    // Vertical line
    mvwvline(w, 3, sort_w, 0, win_h - 4);
    mvwputch(w, 2, sort_w, BORDER_COLOR, LINE_OXXX);
    mvwputch(w, win_h - 1, sort_w, BORDER_COLOR, LINE_XXOX);

    // Header
    mvwprintz(w, 3, 1, c_ltgray, _("<Innermost>"));
    mvwprintz(w, 3, (sort_w - utf8_width(_("(Warmth)"))) / 2, c_ltgray, _("(Warmth)"));
    mvwprintz(w, 3, sort_w - utf8_width(_("Encumbrance")) - 1, c_ltgray, _("Encumbrance"));

    // Footer
    mvwprintz(w, win_h - 2, 1, c_ltgray, _("<Outermost>"));

    wrefresh(w);
}

void draw_description_pane(WINDOW *w, item *worn_item)
{
    it_armor *each_armor = dynamic_cast<it_armor *>(worn_item->type);
    int desc_w = getmaxx(w);
    std::vector<std::string> props = clothing_properties(worn_item, desc_w - 3);
    // Item name
    mvwprintz(w, 0, 1, dam_color[int(worn_item->damage + 1)], each_armor->name.c_str());
    // Layer level
    mvwprintz(w, 1, 2, c_white, "%s",
              worn_item->clothing_lvl_description(true).c_str());

    size_t i;
    for (i = 0; i < props.size(); ++i) {
        mvwprintz(w, i + 3, 2, c_ltgray, props[i].c_str());
    }
    i += 4;

    std::vector<std::string> desc = clothing_flags_description(worn_item);
    if (!desc.empty()) {
        for (size_t j = 0; j < desc.size(); ++j) {
            i += -1 + fold_and_print(w, i + j, 1, desc_w,
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
    props.push_back(name_and_value(_("Storage:"),
                    string_format("%3d", int(each_armor->storage)), width));
    props.push_back(name_and_value(_("Warmth:"),
                    string_format("%3d", int(each_armor->warmth)), width));
    props.push_back("");
    props.push_back(_("Protection:"));
    props.push_back(name_and_value(_("- Bash:"),
                    string_format("%3d", int(worn_item->bash_resist())), width));
    props.push_back(name_and_value(_("- Cut:"),
                    string_format("%3d", int(worn_item->cut_resist())), width));
    props.push_back(name_and_value(_("- Environmental:"),
                    string_format("%3d", int(each_armor->env_resist)), width));
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
