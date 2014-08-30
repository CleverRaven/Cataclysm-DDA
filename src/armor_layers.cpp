#include "player.h"
#include "armor_layers.h"
#include "catacharset.h"
#include "itype.h"
#include "input.h"
#include "output.h"

void player::sort_armor()
{
    it_armor *each_armor = 0;

    /* Define required height of the arrangement pane:
    * + 3 - horizontal lines;
    * + 1 - caption line;
    * + 2 - innermost/outermost string lines;
    * + num_bp - sub-categories (torso, head, eyes, etc.);
    * + 1 - gap;
    * number of lines required for displaying all items is calculated dynamically,
    * because some items can have multiple entries (i.e. cover a few parts of body).
    */
    int req_arrangement_h = 3 + 1 + 2 + num_bp + 1;

    for (int cover = 0; cover < num_bp; cover++) {
        for (auto it = worn.begin(); it != worn.end(); ++it) {
            if (it->covers.test(cover)) {
                req_arrangement_h++;
            }
        }
    }

    /* Define required height of the description pane:
    * + 3 - horizontal lines;
    * + 1 - caption line;
    * + 8 - general properties
    * + 7 - ASSUMPTION: max possible number of flags @ item
    */
    int req_description_h = 3 + 1 + 8 + 7;

    const int win_h = std::min(TERMY, std::max(FULL_SCREEN_HEIGHT, std::max(req_arrangement_h,
                                                                            req_description_h)));

    const int win_w = FULL_SCREEN_WIDTH + (TERMX - FULL_SCREEN_WIDTH) / 3;
    const int win_x = TERMX / 2 - win_w / 2;
    const int win_y = TERMY / 2 - win_h / 2;

    int content_h   = win_h - 6;
    int obsolete_w   = 6;
    int arrangement_w  = (win_w - 3) * 3 / 5;
    int description_w = (win_w - 3) - obsolete_w - arrangement_w;

    int tabindex = num_bp;
    int tabcount = num_bp + 1;

    int obsoleteListSize;
    int obsoleteListIndex  = 0;
    int obsoleteListOffset = 0;
    int selected       = -1;

    int list_size;
    int index  = 0;
    int offset = 0;

    item tmp_item;
    std::vector<item *> tmp_worn;
    std::vector<item *> tmp_arr;
    std::string tmp_str;

    std::string  armor_cat[] = { _("Head"), _("Eyes"), _("Mouth"), _("Torso"),
                                _("L. Arm"), _("R. Arm"), _("L. Hand"), _("R. Hand"),
                                _("L. Leg"), _("R. Leg"), _("L. Foot"), _("R. Foot"),
                                _("All")
                               };

    // Layout window
    WINDOW *w_main = newwin(win_h, win_w, win_y, win_x);
    draw_background(w_main, arrangement_w);

    // Subwindows (between lines)
    WINDOW *w_title       = newwin(1, win_w - 4, win_y + 1, win_x + 2);
    WINDOW *w_description = newwin(content_h, description_w, win_y + 3, win_x + arrangement_w + 1);
    WINDOW *w_arrangement = newwin(content_h, arrangement_w, win_y + 4, win_x);
    WINDOW *w_obsolete    = newwin(content_h, obsolete_w, win_y + 4, win_x + arrangement_w + description_w + 2);

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
        werase(w_obsolete);
        werase(w_description);
        werase(w_arrangement);

        // top bar
        wprintz(w_title, c_white, _("Sort Armor"));
        wprintz(w_title, c_yellow, "  << %s >>", armor_cat[tabindex].c_str());
        tmp_str = string_format(_("Press %s for help."), ctxt.get_desc("HELP").c_str());
        mvwprintz(w_title, 0, win_w - utf8_width(tmp_str.c_str()) - 4,
                  c_white, tmp_str.c_str());

        // Create ptr list of items to display
        tmp_worn.clear();
        if (tabindex == num_bp) { // All
            for (auto it = worn.begin(); it != worn.end(); ++it) {
                tmp_worn.push_back(&*it);
            }
        } else { // bp_*
            for (auto it = worn.begin(); it != worn.end(); ++it) {
                if (it->covers.test(bp_order[tabindex])) {
                    tmp_worn.push_back(&*it);
                }
            }
        }
        obsoleteListSize = ((int)tmp_worn.size() < content_h - 2) ? (int)tmp_worn.size() : content_h - 2;

        // obsolete list
        for (int drawindex = 0; drawindex < obsoleteListSize; drawindex++) {
            int itemindex = obsoleteListOffset + drawindex;
            each_armor = dynamic_cast<it_armor *>(tmp_worn[itemindex]->type);

            if (itemindex == obsoleteListIndex) {
                mvwprintz(w_obsolete, drawindex, 0, c_yellow, ">");
            }

            mvwprintz(w_obsolete, drawindex, 1 + (int)(itemindex == selected), c_dkgray, each_armor->nname(1).c_str());
        }


        tmp_arr.clear();

        // arrangement list
        list_size = 0;
        for (int cover = 0, pos = 0; cover < num_bp; cover++) {
            if (list_size >= offset && pos < content_h) {

                // Category Name
                mvwprintz(w_arrangement, pos, 2, (cover == tabindex) ? c_yellow : c_white,
                          "%s:", armor_cat[ cover ].c_str());

                // Warmth
                if (bp_order[cover] != bp_eyes) {
                    center_print(w_arrangement, pos, bodytemp_color(bp_order[cover]),
                                 "(%d)", warmth(body_part(bp_order[cover])));
                }

                // Encumbrance
                int armorenc = 0;
                double layers = 0.0;
                int enc = encumb(body_part(bp_order[cover]), layers, armorenc);
                std::string enc_details = string_format(" = %d + %d",
                                                        enc - armorenc, armorenc);
                nc_color text_color = ( (enc > 0) ? c_white : c_ltgray);

                // Components of encumbrance
                mvwprintz(w_arrangement, pos, arrangement_w - utf8_width(enc_details.c_str()) - 1,
                          text_color, enc_details.c_str());

                // Total encumbrance
                mvwprintz(w_arrangement, pos, arrangement_w - utf8_width(enc_details.c_str()) - 3,
                          encumb_color(enc), "%2d", enc);
                pos++;
            }
            list_size++;

            // Print all items for current bodypart
            for (std::vector<item>::iterator it = worn.begin(); it != worn.end(); ++it) {
                each_armor = dynamic_cast<it_armor *>(it->type);
                if (it->covers.test(bp_order[cover])) {
                    tmp_arr.push_back(&*it);

                    if (list_size >= offset && pos <= content_h - 1) {

                        int lvl = it->clothing_lvl() + 2;

                        // TODO: update the condition
                        if (index == 0) {
                            mvwhline(w_arrangement, pos, 1, '>', lvl/*, c_yellow*/);
                        }

                        // Item Name
                        mvwprintz(w_arrangement, pos, lvl + 1, dam_color[int(it->damage + 1)],
                                  each_armor->nname(1).c_str());

                        // Storage Value
                        int storage_val = int(each_armor->storage);
                        if (storage_val > 0) {
                            wprintz(w_arrangement, c_ltgray, " [%d]", storage_val);
                        }

                        // Item Encumbrance Value
                        mvwprintz(w_arrangement, pos, arrangement_w - 2, c_ltgray, "%d",
                                  (it->has_flag("FIT")) ? std::max(0, int(each_armor->encumber) - 1)
                                  : int(each_armor->encumber));

                        pos++;
                    }
                    list_size++;
                }
            }
        }

        // Items stats
        if (!tmp_worn.empty()) {
            draw_description_pane(w_description, tmp_worn[obsoleteListIndex]);
        } else {
            mvwprintz(w_description, 0, 1, c_white, _("Nothing to see here!"));
        }

        // ScrollBar
        draw_scrollbar(w_arrangement, offset /*<--temp*/, content_h, list_size, 0, 0);

        // Scroll-up is available
        if (offset > 0) {
            mvwprintz(w_arrangement, content_h - 1, arrangement_w - 6, c_yellow, "^");
        }

        // Scroll-down is available
        if (list_size > content_h - 1) {
            mvwprintz(w_arrangement, content_h - 1, arrangement_w - 5, c_yellow, "v");
        }

        // F5
        wrefresh(w_title);
        wrefresh(w_obsolete);
        wrefresh(w_description);
        wrefresh(w_arrangement);

        const std::string action = ctxt.handle_input();
        if (action == "UP" && obsoleteListSize > 0) {
            offset--;
            if (offset < 0) {
                offset = 0;
            }


            obsoleteListIndex--;
            if (obsoleteListIndex < 0) {
                obsoleteListIndex = tmp_worn.size() - 1;
            }

            // Scrolling logic
            obsoleteListOffset = (obsoleteListIndex < obsoleteListOffset) ? obsoleteListIndex : obsoleteListOffset;
            if (!((obsoleteListIndex >= obsoleteListOffset) && (obsoleteListIndex < obsoleteListOffset + obsoleteListSize))) {
                obsoleteListOffset = obsoleteListIndex - obsoleteListSize + 1;
                obsoleteListOffset = (obsoleteListOffset > 0) ? obsoleteListOffset : 0;
            }

            // move selected item
            if (selected >= 0) {
                tmp_item = *tmp_worn[obsoleteListIndex];
                for (auto it = worn.begin(); it != worn.end(); ++it) {
                    if (&*it == tmp_worn[obsoleteListIndex]) {
                        *it = *tmp_worn[selected];
                    }
                }

                for (auto it = worn.begin(); it != worn.end(); ++it) {
                    if (&*it == tmp_worn[selected]) {
                        *it = tmp_item;
                    }
                }

                selected = obsoleteListIndex;
            }
        } else if (action == "DOWN" && obsoleteListSize > 0) {
            offset++;
            if (offset + content_h > list_size) {
                offset = list_size - content_h;
            }



            obsoleteListIndex = (obsoleteListIndex + 1) % tmp_worn.size();

            // Scrolling logic
            if (!((obsoleteListIndex >= obsoleteListOffset) && (obsoleteListIndex < obsoleteListOffset + obsoleteListSize))) {
                obsoleteListOffset = obsoleteListIndex - obsoleteListSize + 1;
                obsoleteListOffset = (obsoleteListOffset > 0) ? obsoleteListOffset : 0;
            }

            // move selected item
            if (selected >= 0) {
                tmp_item = *tmp_worn[obsoleteListIndex];
                for (auto it = worn.begin(); it != worn.end(); ++it) {
                    if (&*it == tmp_worn[obsoleteListIndex]) {
                        *it = *tmp_worn[selected];
                    }
                }

                for (auto it = worn.begin(); it != worn.end(); ++it) {
                    if (&*it == tmp_worn[selected]) {
                        *it = tmp_item;
                    }
                }
                selected = obsoleteListIndex;
            }
        } else if (action == "LEFT") {
            tabindex--;
            if (tabindex < 0) {
                tabindex = tabcount - 1;
            }
            obsoleteListIndex = obsoleteListOffset = 0;
            selected = -1;
        } else if (action == "RIGHT") {
            tabindex = (tabindex + 1) % tabcount;
            obsoleteListIndex = obsoleteListOffset = 0;
            selected = -1;
        } else if (action == "NEXT_TAB") {
            // back-up for mode switches

        } else if (action == "PREV_TAB") {
            // back-up for mode switches

        } else if (action == "MOVE_ARMOR") {
            if (selected >= 0) {
                selected = -1;
            } else {
                selected = obsoleteListIndex;
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
                } else if (invlet_to_position(invlet) != INT_MIN) {
                    invlet_index--;
                } else {
                    w.invlet = invlet;
                    worn_index--;
                    invlet_index--;
                }
            }
        } else if (action == "HELP") {
            /* Not enough space in Help window for this:
\n\
[Encumbrance]: \n\
Total value of encumbrance is shown in the left side of equation. \n\
The first component of equation is the encumbrance caused by the number of clothing on that bodypart, \
and the second one is the summed encumbrance from all clothing on that bodypart.
            */
            popup_getkey(_("\
[General Description]: \n\
Indention of name of the item represents the clothing layer: \n\
>>underwear\n\
>>>regular layer\n\
>>>>outerwear\n\
>>>>>belted layer\n\
 \n\
Color of the item represents its condition: \
green color means it's a brand new item, red color - a torn one.\n\
 \n\
Order of the items in the list for each bodypart is important: upper in the list means it closer to the body.\n\
 \n\
[Keybinding]: \n\
Use the arrow- or keypad keys to navigate the list of items. \n\
Press %s to select highlighted armor for reordering.\n\
Press %s / %s to switch types of represented information. \n\
Press %s to assign special inventory letters to clothing.\n"),
                         ctxt.get_desc("MOVE_ARMOR").c_str(),
                         ctxt.get_desc("PREV_TAB").c_str(),
                         ctxt.get_desc("NEXT_TAB").c_str(),
                         ctxt.get_desc("ASSIGN_INVLETS").c_str()
                        );
            draw_background(w_main, arrangement_w);

        } else if (action == "QUIT") {
            sorting = false;
        }
    }

    delwin(w_title);
    delwin(w_obsolete);
    delwin(w_description);
    delwin(w_arrangement);
    delwin(w_main);
}

void draw_background(WINDOW *w, int sort_w)
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

void draw_description_pane(WINDOW *w_description, item *worn_item)
{
    it_armor *each_armor = dynamic_cast<it_armor *>(worn_item->type);

    // Item Name
    mvwprintz(w_description, 0, 1, dam_color[int(worn_item->damage + 1)], each_armor->nname(1).c_str());

    // Layer
    mvwprintz(w_description, 1, 2, c_white, worn_item->clothing_lvl_description(true).c_str());

    int description_w = getmaxx(w_description);
    size_t i;

    // Item Properties List
    std::vector<std::string> props = clothing_properties(worn_item, description_w - 3);
    for (i = 0; i < props.size(); ++i) {
        mvwprintz(w_description, i + 3, 2, c_ltgray, props[i].c_str());
    }

    i += 4;
    i += fold_and_print(w_description, i, 0, description_w, c_ltblue,
                        "%s", clothing_layer(worn_item).c_str());

    // Item Description (based on flags)
    std::vector<std::string> desc = clothing_flags_description(worn_item);
    if (!desc.empty()) {
        for (size_t j = 0; j < desc.size(); ++j) {
            i += -1 + fold_and_print(w_description, i + j, 0, description_w,
                                     c_ltblue, "%s", desc[j].c_str());
        }
    }
}

std::string clothing_layer(item *worn_item)
{
    std::string layer = "";

    if (worn_item->has_flag("SKINTIGHT")) {
        layer = _("This is worn next to the skin.");
    } else if (worn_item->has_flag("OUTER")) {
        layer = _("This is worn over your other clothes.");
    } else if (worn_item->has_flag("BELTED")) {
        layer = _("It is the belted layer.");
    }

    return layer;
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
    props.push_back(name_and_value(_("Warmth:"),
                                   string_format("%3d", int(each_armor->warmth)), width));
    props.push_back(name_and_value(_("Storage:"),
                                   string_format("%3d", int(each_armor->storage)), width));
    props.push_back("");
    props.push_back("Protection:");
    props.push_back(name_and_value(_(" - Bash:"),
                                   string_format("%3d", int(worn_item->bash_resist())), width));
    props.push_back(name_and_value(_(" - Cut:"),
                                   string_format("%3d", int(worn_item->cut_resist())), width));
    props.push_back(name_and_value(_(" - Environmental:"),
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
