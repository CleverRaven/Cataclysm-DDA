#include "player.h"
#include "armor_layers.h"
#include "catacharset.h"
#include "input.h"
#include "output.h"

void player::sort_armor()
{
    /* Define required height of the right pane:
    * + 3 - horizontal lines;
    * + 1 - caption line;
    * + 2 - innermost/outermost string lines;
    * + 12 - sub-categories (torso, head, eyes, etc.);
    * + 1 - gap;
    * number of lines required for displaying all items is calculated dynamically,
    * because some items can have multiple entries (i.e. cover a few parts of body).
    */

    int req_right_h = 3 + 1 + 2 + 12 + 1;
    for (int cover = 0; cover < num_bp; cover++) {
        for( auto &elem : worn ) {
            if( elem.covers( static_cast<body_part>( cover ) ) ) {
                req_right_h++;
            }
        }
    }

    /* Define required height of the mid pane:
    * + 3 - horizontal lines;
    * + 1 - caption line;
    * + 8 - general properties
    * + 7 - ASSUMPTION: max possible number of flags @ item
    * + 13 - warmth & enc block
    */
    int req_mid_h = 3 + 1 + 8 + 7 + 13;

    const int win_h = std::min(TERMY, std::max(FULL_SCREEN_HEIGHT,
                               std::max(req_right_h, req_mid_h)));
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

    std::vector<item *> tmp_worn;
    std::string tmp_str;

    std::string  armor_cat[] = {_("Torso"), _("Head"), _("Eyes"), _("Mouth"), _("L. Arm"), _("R. Arm"),
                                _("L. Hand"), _("R. Hand"), _("L. Leg"), _("R. Leg"), _("L. Foot"),
                                _("R. Foot"), _("All")
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
        werase(w_sort_cat);
        werase(w_sort_left);
        werase(w_sort_middle);
        werase(w_sort_right);

        // top bar
        wprintz(w_sort_cat, c_white, _("Sort Armor"));
        wprintz(w_sort_cat, c_yellow, "  << %s >>", armor_cat[tabindex].c_str());
        tmp_str = string_format(_("Press %s for help."), ctxt.get_desc("HELP").c_str());
        mvwprintz(w_sort_cat, 0, win_w - utf8_width(tmp_str.c_str()) - 4,
                  c_white, tmp_str.c_str());

        // Create ptr list of items to display
        tmp_worn.clear();
        if (tabindex == 12) { // All
            for( auto &elem : worn ) {
                tmp_worn.push_back( &elem );
            }
        } else { // bp_*
            for( auto &elem : worn ) {
                if( elem.covers( static_cast<body_part>( tabindex ) ) ) {
                    tmp_worn.push_back( &elem );
                }
            }
        }
        leftListSize = ((int)tmp_worn.size() < cont_h - 2) ? (int)tmp_worn.size() : cont_h - 2;

        // Left header
        mvwprintz(w_sort_left, 0, 0, c_ltgray, _("(Innermost)"));
        mvwprintz(w_sort_left, 0, left_w - utf8_width(_("Storage")), c_ltgray, _("Storage"));

        // Left list
        for (int drawindex = 0; drawindex < leftListSize; drawindex++) {
            int itemindex = leftListOffset + drawindex;

            if (itemindex == leftListIndex) {
                mvwprintz(w_sort_left, drawindex + 1, 0, c_yellow, ">>");
            }

            if (itemindex == selected) {
                mvwprintz(w_sort_left, drawindex + 1, 3, dam_color[int(tmp_worn[itemindex]->damage + 1)],
                          tmp_worn[itemindex]->type_name(1).c_str());
            } else {
                mvwprintz(w_sort_left, drawindex + 1, 2, dam_color[int(tmp_worn[itemindex]->damage + 1)],
                          tmp_worn[itemindex]->type_name(1).c_str());
            }
            mvwprintz(w_sort_left, drawindex + 1, left_w - 3, c_ltgray, "%3d", tmp_worn[itemindex]->get_storage());
        }

        // Left footer
        mvwprintz(w_sort_left, cont_h - 1, 0, c_ltgray, _("(Outermost)"));
        if (leftListSize > (int)tmp_worn.size()) {
            mvwprintz(w_sort_left, cont_h - 1, left_w - utf8_width(_("<more>")), c_ltblue, _("<more>"));
        }
        if (leftListSize == 0) {
            mvwprintz(w_sort_left, cont_h - 1, left_w - utf8_width(_("<empty>")), c_ltblue, _("<empty>"));
        }

        // Items stats
        if (leftListSize) {
            draw_mid_pane(w_sort_middle, tmp_worn[leftListIndex]);
        } else {
            mvwprintz(w_sort_middle, 0, 1, c_white, _("Nothing to see here!"));
        }

        // Player encumbrance - altered copy of '@' screen
        mvwprintz(w_sort_middle, cont_h - 13, 1, c_white, _("Encumbrance and Warmth"));
        for (int i = 0; i < num_bp; ++i) {
            int enc, armorenc;
            double layers;
            layers = armorenc = 0;
            enc = encumb(body_part(i), layers, armorenc);
            if (leftListSize && (tmp_worn[leftListIndex]->covers(static_cast<body_part>(i)))) {
                mvwprintz(w_sort_middle, cont_h - 12 + i, 2, c_green, "%s:", armor_cat[i].c_str());
            } else {
                mvwprintz(w_sort_middle, cont_h - 12 + i, 2, c_ltgray, "%s:", armor_cat[i].c_str());
            }
            mvwprintz(w_sort_middle, cont_h - 12 + i, middle_w - 16, c_ltgray, "%d+%d = ", armorenc,
                      enc - armorenc);
            wprintz(w_sort_middle, encumb_color(enc), "%d" , enc);
            int bodyTempInt = (temp_conv[i] / 100.0) * 2 - 100; // Scale of -100 to +100
            mvwprintz(w_sort_middle, cont_h - 12 + i, middle_w - 6,
                      bodytemp_color(i), "(%3d)", bodyTempInt);
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
            for( auto &elem : worn ) {
                if( elem.covers( static_cast<body_part>( cover ) ) ) {
                    if (rightListSize >= rightListOffset && pos <= cont_h - 2) {
                        mvwprintz( w_sort_right, pos, 2, dam_color[int( elem.damage + 1 )],
                                   elem.type_name( 1 ).c_str() );
                        mvwprintz( w_sort_right, pos, right_w - 2, c_ltgray, "%d",
                                   ( elem.has_flag( "FIT" ) ) ?
                                       std::max( 0, elem.get_encumber() - 1 ) :
                                       elem.get_encumber() );
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
                if( leftListIndex < selected ) {
                    std::swap( *tmp_worn[leftListIndex], *tmp_worn[selected] );
                } else {
                    const auto tmp_item = *tmp_worn[selected];
                    const auto it_selected = worn.begin() + ( tmp_worn[selected] - &worn.front() );
                    worn.erase( it_selected );
                    worn.insert( worn.end(), tmp_item );
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
                if( leftListIndex > selected ) {
                    std::swap( *tmp_worn[leftListIndex], *tmp_worn[selected] );
                } else {
                    const auto tmp_item = *tmp_worn[selected];
                    const auto it_selected = worn.begin() + ( tmp_worn[selected] - &worn.front() );
                    worn.erase( it_selected );
                    worn.insert( worn.begin(), tmp_item );
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
                } else if (invlet_to_position(invlet) != INT_MIN) {
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
The first number is the summed encumbrance from all clothing on that bodypart.\n\
The second number is the encumbrance caused by the number of clothing on that bodypart.\n\
The sum of these values is the effective encumbrance value your character has for that bodypart."),
                         ctxt.get_desc("MOVE_ARMOR").c_str(),
                         ctxt.get_desc("PREV_TAB").c_str(),
                         ctxt.get_desc("NEXT_TAB").c_str(),
                         ctxt.get_desc("ASSIGN_INVLETS").c_str()
                        );
            //TODO: refresh the window properly. Current method erases the intersection symbols
            draw_border(w_sort_armor); // hack to mark whole window for redrawing
            wrefresh(w_sort_armor);
        } else if (action == "QUIT") {
            sorting = false;
        }
    }

    delwin(w_sort_cat);
    delwin(w_sort_left);
    delwin(w_sort_middle);
    delwin(w_sort_right);
    delwin(w_sort_armor);
}

void draw_mid_pane(WINDOW *w_sort_middle, item *worn_item)
{
    mvwprintz(w_sort_middle, 0, 1, c_white, worn_item->type_name(1).c_str());
    int middle_w = getmaxx(w_sort_middle);
    std::vector<std::string> props = clothing_properties(worn_item, middle_w - 3);
    size_t i;
    for (i = 0; i < props.size(); ++i) {
        mvwprintz(w_sort_middle, i + 1, 2, c_ltgray, props[i].c_str());
    }

    i += 2;
    i += fold_and_print(w_sort_middle, i, 0, middle_w, c_ltblue,
                        "%s", clothing_layer(worn_item).c_str());

    std::vector<std::string> desc = clothing_flags_description(worn_item);
    if (!desc.empty()) {
        for (size_t j = 0; j < desc.size(); ++j) {
            i += -1 + fold_and_print(w_sort_middle, i + j, 0, middle_w,
                                     c_ltblue, "%s", desc[j].c_str());
        }
    }
}

std::string clothing_layer(item *worn_item)
{
    std::string layer = "";

    if (worn_item->has_flag("SKINTIGHT")) {
        layer = _("This is worn next to the skin.");
    } else if (worn_item->has_flag("WAIST")) {
        layer = _("This is worn on or around your waist.");
    } else if (worn_item->has_flag("OUTER")) {
        layer = _("This is worn over your other clothes.");
    } else if (worn_item->has_flag("BELTED")) {
        layer = _("This is strapped onto you.");
    }

    return layer;
}

std::vector<std::string> clothing_properties(item *worn_item, int width)
{
    std::vector<std::string> props;
    props.push_back(name_and_value(_("Coverage:"),
                                   string_format("%3d", worn_item->get_coverage()), width));
    props.push_back(name_and_value(_("Encumbrance:"), string_format("%3d",
                                   (worn_item->has_flag("FIT")) ? std::max(0, worn_item->get_encumber() - 1) :
                                   worn_item->get_encumber()), width));
    props.push_back(name_and_value(_("Bash Protection:"),
                                   string_format("%3d", int(worn_item->bash_resist())), width));
    props.push_back(name_and_value(_("Cut Protection:"),
                                   string_format("%3d", int(worn_item->cut_resist())), width));
    props.push_back(name_and_value(_("Warmth:"),
                                   string_format("%3d", worn_item->get_warmth()), width));
    props.push_back(name_and_value(_("Storage:"),
                                   string_format("%3d", worn_item->get_storage()), width));

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
