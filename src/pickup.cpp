#include "auto_pickup.h"
#include "game.h"
#include "messages.h"
#include "pickup.h"

#include <map>
#include <vector>

//Pickup object
static Pickup pickup_obj;

// Pick up items at (posx, posy).
void Pickup::pick_up(int posx, int posy, int min)
{
    //min == -1 is Autopickup

    if (g->m.has_flag("SEALED", posx, posy)) {
        return;
    }

    g->write_msg();
    if (!g->u.can_pickup(min != -1)) { // no message on autopickup (-1)
        return;
    }

    bool weight_is_okay = (g->u.weight_carried() <= g->u.weight_capacity());
    bool volume_is_okay = (g->u.volume_carried() <= g->u.volume_capacity() -  2);
    pickup_obj.from_veh = false;
    int veh_root_part = 0;

    int k_part = 0;
    int wtr_part = 0;
    int w_part = 0;
    int craft_part = 0;
    int cargo_part = 0;
    int chempart = 0;
    int ctrl_part = 0;
    std::vector<std::string> menu_items;
    std::vector<uimenu_entry> options_message;

    vehicle *veh = g->m.veh_at (posx, posy, veh_root_part);
    std::vector<item> here_ground = g->m.i_at(posx, posy);
    if (min != -1 && veh) {
        k_part = veh->part_with_feature(veh_root_part, "KITCHEN");
        wtr_part = veh->part_with_feature(veh_root_part, "FAUCET");
        w_part = veh->part_with_feature(veh_root_part, "WELDRIG");
        craft_part = veh->part_with_feature(veh_root_part, "CRAFTRIG");
        chempart = veh->part_with_feature(veh_root_part, "CHEMLAB");
        cargo_part = veh->part_with_feature(veh_root_part, "CARGO", false);
        ctrl_part = veh->part_with_feature(veh_root_part, "CONTROLS");
        pickup_obj.from_veh = veh && cargo_part >= 0 && !veh->parts[cargo_part].items.empty();

        menu_items.push_back(_("Examine vehicle"));
        options_message.push_back(uimenu_entry(_("Examine vehicle"), 'e'));
        if (ctrl_part >= 0) {
            menu_items.push_back(_("Control vehicle"));
            options_message.push_back(uimenu_entry(_("Control vehicle"), 'v'));
        }

        if (pickup_obj.from_veh) {
            menu_items.push_back(_("Get items"));
            options_message.push_back(uimenu_entry(_("Get items"), 'g'));
        }

        if(!here_ground.empty()) {
            menu_items.push_back(_("Get items on the ground"));
            options_message.push_back(uimenu_entry(_("Get items on the ground"), 'i'));
        }

        if((k_part >= 0 || chempart >= 0) && veh->fuel_left("battery") > 0) {
            menu_items.push_back(_("Use the hotplate"));
            options_message.push_back(uimenu_entry(_("Use the hotplate"), 'h'));
        }
        if((k_part >= 0 || wtr_part >= 0) && veh->fuel_left("water") > 0) {
            menu_items.push_back(_("Fill a container with water"));
            options_message.push_back(uimenu_entry(_("Fill a container with water"), 'c'));

            menu_items.push_back(_("Have a drink"));
            options_message.push_back(uimenu_entry(_("Have a drink"), 'd'));
        }
        if(w_part >= 0 && veh->fuel_left("battery") > 0) {
            menu_items.push_back(_("Use the welding rig?"));
            options_message.push_back(uimenu_entry(_("Use the welding rig?"), 'w'));
        }
        if(craft_part >= 0 && veh->fuel_left("battery") > 0) {
            menu_items.push_back(_("Use the water purifier?"));
            options_message.push_back(uimenu_entry(_("Use the water purifier?"), 'p'));
        }

        int choice;
        if( menu_items.size() == 1 ) {
            choice = 0;
        } else {
            uimenu selectmenu;
            selectmenu.return_invalid = true;
            selectmenu.text = _("Select an action");
            selectmenu.entries = options_message;
            selectmenu.selected = 0;
            selectmenu.query();
            choice = selectmenu.ret;
        }

        if(choice < 0) {
            return;
        }
        if(menu_items[choice] == _("Use the hotplate")) {
            //Will be -1 if no battery at all
            item tmp_hotplate( "hotplate", 0 );
            // Drain a ton of power
            tmp_hotplate.charges = veh->drain( "battery", 100 );
            if( tmp_hotplate.is_tool() ) {
                it_tool *tmptool = dynamic_cast<it_tool *>((&tmp_hotplate)->type);
                if ( tmp_hotplate.charges >= tmptool->charges_per_use ) {
                    tmptool->invoke(&g->u, &tmp_hotplate, false);
                    tmp_hotplate.charges -= tmptool->charges_per_use;
                    veh->refill( "battery", tmp_hotplate.charges );
                }
            }
            return;
        }

        if(menu_items[choice] == _("Fill a container with water")) {
            int amt = veh->drain("water", veh->fuel_left("water"));
            item fill_water( default_ammo("water"), calendar::turn );
            fill_water.charges = amt;
            int back = g->move_liquid(fill_water);
            if (back >= 0) {
                veh->refill("water", back);
            } else {
                veh->refill("water", amt);
            }
            return;
        }

        if(menu_items[choice] == _("Have a drink")) {
            veh->drain("water", 1);
            item water( "water_clean", 0 );
            g->u.eat(&water, dynamic_cast<it_comest *>(water.type));
            g->u.moves -= 250;
            return;
        }

        if(menu_items[choice] == _("Use the welding rig?")) {
            //Will be -1 if no battery at all
            item tmp_welder( "welder", 0 );
            // Drain a ton of power
            tmp_welder.charges = veh->drain( "battery", 1000 );
            if( tmp_welder.is_tool() ) {
                it_tool *tmptool = dynamic_cast<it_tool *>((&tmp_welder)->type);
                if ( tmp_welder.charges >= tmptool->charges_per_use ) {
                    tmptool->invoke( &g->u, &tmp_welder, false );
                    tmp_welder.charges -= tmptool->charges_per_use;
                    veh->refill( "battery", tmp_welder.charges );
                }
            }
            return;
        }

        if(menu_items[choice] == _("Use the water purifier?")) {
            //Will be -1 if no battery at all
            item tmp_purifier( "water_purifier", 0 );
            // Drain a ton of power
            tmp_purifier.charges = veh->drain( "battery", 100 );
            if( tmp_purifier.is_tool() ) {
                it_tool *tmptool = dynamic_cast<it_tool *>((&tmp_purifier)->type);
                if ( tmp_purifier.charges >= tmptool->charges_per_use ) {
                    tmptool->invoke( &g->u, &tmp_purifier, false );
                    tmp_purifier.charges -= tmptool->charges_per_use;
                    veh->refill( "battery", tmp_purifier.charges );
                }
            }
            return;
        }

        if(menu_items[choice] == _("Control vehicle")) {
            veh->use_controls();
            return;
        }

        if(menu_items[choice] == _("Examine vehicle")) {
            g->exam_vehicle(*veh, posx, posy);
            return;
        }

        if(menu_items[choice] == _("Get items on the ground")) {
            pickup_obj.from_veh = false;
        }

    }

    if (!pickup_obj.from_veh) {
        bool isEmpty = (g->m.i_at(posx, posy).empty());

        // Hide the pickup window if this is a toilet and there's nothing here
        // but water.
        if ((!isEmpty) && g->m.furn(posx, posy) == f_toilet) {
            isEmpty = true;
            for (size_t i = 0; isEmpty && i < g->m.i_at(posx, posy).size(); i++) {
                if (g->m.i_at(posx, posy)[i].typeId() != "water") {
                    isEmpty = false;
                }
            }
        }

        if (isEmpty) {
            return;
        }
    }

    // which items are we grabbing?
    std::vector<item> here = pickup_obj.from_veh ? veh->parts[cargo_part].items : g->m.i_at(posx, posy);

    // Not many items, just grab them
    if (here.size() <= min && min != -1) {
        item newit = here[0];
        int moves_taken = 100;
        bool picked_up = false;

        if (newit.made_of(LIQUID)) {
            add_msg(m_info, _("You can't pick up a liquid!"));
            return;
        }
        newit.invlet = g->u.inv.get_invlet_for_item( newit.typeId() );
        if (!g->u.can_pickWeight(newit.weight(), false)) {
            add_msg(m_info, _("The %s is too heavy!"), newit.display_name().c_str());
        } else if (!g->u.can_pickVolume(newit.volume())) {
            if (newit.is_ammo() && (newit.ammo_type() == "arrow" || newit.ammo_type() == "bolt")) {
                handle_quiver_insertion(newit, false, moves_taken, picked_up);
                if (newit.charges > 0) {
                    add_msg(m_info, ngettext("There's no room in your inventory for the %s.",
                                             "There's no room in your inventory for the %ss.",
                                             newit.charges), newit.name.c_str());
                }
            } else if (g->u.is_armed()) {
                if (!g->u.weapon.has_flag("NO_UNWIELD")) {
                    // Armor can be instantly worn
                    if (newit.is_armor() &&
                        query_yn(_("Put on the %s?"),
                                 newit.display_name().c_str())) {
                        if (g->u.wear_item(&newit)) {
                            picked_up = true;
                        }
                    } else if (query_yn(_("Drop your %s and pick up %s?"),
                                        g->u.weapon.display_name().c_str(),
                                        newit.display_name().c_str())) {
                        picked_up = true;
                        g->m.add_item_or_charges(posx, posy, g->u.remove_weapon(), 1);
                        g->u.inv.assign_empty_invlet(newit, true);  // force getting an invlet.
                        g->u.wield(&(g->u.i_add(newit)));
                        add_msg(m_info, _("Wielding %c - %s"), newit.invlet,
                                newit.display_name().c_str());
                    }
                } else {
                    add_msg(m_info, _("There's no room in your inventory for the %s, "
                                      "and you can't unwield your %s."),
                            newit.display_name().c_str(),
                            g->u.weapon.display_name().c_str());
                }
            } else {
                g->u.inv.assign_empty_invlet(newit, true);  // force getting an invlet.
                g->u.wield(&(g->u.i_add(newit)));
                picked_up = true;
                add_msg(m_info, _("Wielding %c - %s"), newit.invlet, newit.display_name().c_str());
            }
        } else if (newit.is_ammo() && (newit.ammo_type() == "arrow" || newit.ammo_type() == "bolt")) {
            //add ammo to quiver
            handle_quiver_insertion(newit, true, moves_taken, picked_up);
        } else if (!g->u.is_armed() &&
                   (g->u.volume_carried() + newit.volume() > g->u.volume_capacity() - 2 ||
                    newit.is_weap() || newit.is_gun())) {
            g->u.weapon = newit;
            picked_up = true;
            add_msg(m_info, _("Wielding %c - %s"), newit.invlet, newit.display_name().c_str());
        } else {
            newit = g->u.i_add(newit);
            picked_up = true;
            add_msg(m_info, "%c - %s", newit.invlet == 0 ?
                    ' ' : newit.invlet, newit.display_name().c_str());
        }

        if(picked_up) {
            pickup_obj.remove_from_map_or_vehicle(posx, posy, veh, cargo_part, moves_taken, 0);
        }

        if (weight_is_okay && g->u.weight_carried() >= g->u.weight_capacity()) {
            add_msg(m_bad, _("You're overburdened!"));
        }
        if (volume_is_okay && g->u.volume_carried() > g->u.volume_capacity() - 2) {
            add_msg(m_bad, _("You struggle to carry such a large volume!"));
        }
        return;
    }

    if(min != -1) { // don't bother if we're just autopickup-ing
        g->temp_exit_fullscreen();
    }
    bool sideStyle = use_narrow_sidebar();

    // Otherwise, we have Autopickup, 2 or more items and should list them, etc.
    int maxmaxitems = sideStyle ? TERMY : getmaxy(g->w_messages) - 3;

    int itemsH = std::min(25, TERMY / 2);
    int pickupBorderRows = 3;

    // The pickup list may consume the entire terminal, minus space needed for its
    // header/footer and the item info window.
    int minleftover = itemsH + pickupBorderRows;
    if(maxmaxitems > TERMY - minleftover) {
        maxmaxitems = TERMY - minleftover;
    }

    const int minmaxitems = sideStyle ? 6 : 9;

    std::vector<bool> getitem;
    getitem.resize(here.size(), false);

    int maxitems = here.size();
    maxitems = (maxitems < minmaxitems ? minmaxitems : (maxitems > maxmaxitems ? maxmaxitems :
                maxitems ));

    int pickupH = maxitems + pickupBorderRows;
    int pickupW = getmaxx(g->w_messages);
    int pickupY = VIEW_OFFSET_Y;
    int pickupX = getbegx(g->w_messages);

    int itemsW = pickupW;
    int itemsY = sideStyle ? pickupY + pickupH : TERMY - itemsH;
    int itemsX = pickupX;

    WINDOW *w_pickup    = newwin(pickupH, pickupW, pickupY, pickupX);
    WINDOW *w_item_info = newwin(itemsH,  itemsW,  itemsY,  itemsX);

    int ch = ' ';
    int start = 0, cur_it;
    int new_weight = g->u.weight_carried(), new_volume = g->u.volume_carried();
    bool update = true;
    mvwprintw(w_pickup, 0, 0, _("PICK UP"));
    int selected = 0;
    int last_selected = -1;

    int itemcount = 0;
    std::map<int, unsigned int> pickup_count; // Count of how many we'll pick up from each stack

    if (min == -1) { //Auto Pickup, select matching items
        bool bFoundSomething = false;

        //Loop through Items lowest Volume first
        bool bPickup = false;

        for(size_t iVol = 0, iNumChecked = 0; iNumChecked < here.size(); iVol++) {
            for (size_t i = 0; i < here.size(); i++) {
                bPickup = false;
                if (here[i].volume() == iVol) {
                    iNumChecked++;

                    //Auto Pickup all items with 0 Volume and Weight <= AUTO_PICKUP_ZERO * 50
                    if (OPTIONS["AUTO_PICKUP_ZERO"]) {
                        if (here[i].volume() == 0 &&
                            here[i].weight() <= OPTIONS["AUTO_PICKUP_ZERO"] * 50) {
                            bPickup = true;
                        }
                    }

                    //Check the Pickup Rules
                    if ( mapAutoPickupItems[here[i].tname()] == "true" ) {
                        bPickup = true;
                    } else if ( mapAutoPickupItems[here[i].tname()] != "false" ) {
                        //No prematched pickup rule found
                        //items with damage, (fits) or a container
                        createPickupRules(here[i].tname());

                        if ( mapAutoPickupItems[here[i].tname()] == "true" ) {
                            bPickup = true;
                        }
                    }
                }

                if (bPickup) {
                    getitem[i] = bPickup;
                    bFoundSomething = true;
                }
            }
        }

        if (!bFoundSomething) {
            return;
        }
    } else {
        if(g->was_fullscreen) {
            g->draw_ter();
            g->write_msg();
        }
        // Now print the two lists; those on the ground and about to be added to inv
        // Continue until we hit return or space
        do {
            static const std::string pickup_chars =
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:;";
            size_t idx = -1;
            for (int i = 1; i < pickupH; i++) {
                mvwprintw(w_pickup, i, 0,
                          "                                                ");
            }
            if (ch >= '0' && ch <= '9') {
                ch = (char)ch - '0';
                itemcount *= 10;
                itemcount += ch;
            } else if ((ch == '<' || ch == KEY_PPAGE) && start > 0) {
                start -= maxitems;
                selected = start;
                mvwprintw(w_pickup, maxitems + 2, 0, "         ");
            } else if ((ch == '>' || ch == KEY_NPAGE) && start + maxitems < here.size()) {
                start += maxitems;
                selected = start;
                mvwprintw(w_pickup, maxitems + 2, pickupH, "            ");
            } else if ( ch == KEY_UP ) {
                selected--;
                if ( selected < 0 ) {
                    selected = here.size() - 1;
                    start = (int)( here.size() / maxitems ) * maxitems;
                    if (start >= here.size()) {
                        start -= maxitems;
                    }
                } else if ( selected < start ) {
                    start -= maxitems;
                }
            } else if ( ch == KEY_DOWN ) {
                selected++;
                if ( selected >= here.size() ) {
                    selected = 0;
                    start = 0;
                } else if ( selected >= start + maxitems ) {
                    start += maxitems;
                }
            } else if ( selected >= 0 && (
                            ( ch == KEY_RIGHT && !getitem[selected]) ||
                            ( ch == KEY_LEFT && getitem[selected] )
                        ) ) {
                idx = selected;
            } else if ( ch == '`' ) {
                std::string ext = string_input_popup(_("Enter 2 letters (case sensitive):"), 2);
                if(ext.size() == 2) {
                    int p1 = pickup_chars.find(ext.at(0));
                    int p2 = pickup_chars.find(ext.at(1));
                    if ( p1 != -1 && p2 != -1 ) {
                        idx = pickup_chars.size() + ( p1 * pickup_chars.size() ) + p2;
                    }
                }
            } else {
                idx = pickup_chars.find(ch);
            }

            if ( idx < here.size()) {
                if (idx != -1) {
                    if (itemcount != 0 || pickup_count[idx] == 0) {
                        if (itemcount >= here[idx].charges || !here[idx].count_by_charges()) {
                            // Ignore the count if we pickup the whole stack anyway
                            // or something that is not counted by charges (tools)
                            itemcount = 0;
                        }
                        pickup_count[idx] = itemcount;
                        itemcount = 0;
                    }
                }

                getitem[idx] = ( ch == KEY_RIGHT ? true : ( ch == KEY_LEFT ? false : !getitem[idx] ) );
                if ( ch != KEY_RIGHT && ch != KEY_LEFT) {
                    selected = idx;
                    start = (int)( idx / maxitems ) * maxitems;
                }

                if (getitem[idx]) {
                    if (pickup_count[idx] != 0 &&
                        pickup_count[idx] < here[idx].charges) {
                        item temp = here[idx].clone();
                        temp.charges = pickup_count[idx];
                        new_weight += temp.weight();
                        new_volume += temp.volume();
                    } else {
                        new_weight += here[idx].weight();
                        new_volume += here[idx].volume();
                    }
                } else if (pickup_count[idx] != 0 &&
                           pickup_count[idx] < here[idx].charges) {
                    item temp = here[idx].clone();
                    temp.charges = pickup_count[idx];
                    new_weight -= temp.weight();
                    new_volume -= temp.volume();
                    pickup_count[idx] = 0;
                } else {
                    new_weight -= here[idx].weight();
                    new_volume -= here[idx].volume();
                }
                update = true;
            }

            if ( selected != last_selected ) {
                last_selected = selected;
                werase(w_item_info);
                if ( selected >= 0 && selected <= here.size() - 1 ) {
                    std::vector<iteminfo> vThisItem, vDummy;
                    here[selected].info(true, &vThisItem);

                    draw_item_info(w_item_info, "", vThisItem, vDummy, 0, true, true);
                }
                draw_border(w_item_info);
                mvwprintz(w_item_info, 0, 2, c_white, "< %s >", here[selected].display_name().c_str());
                wrefresh(w_item_info);
            }

            if (ch == ',') {
                int count = 0;
                for (size_t i = 0; i < here.size(); i++) {
                    if (getitem[i]) {
                        count++;
                    } else {
                        new_weight += here[i].weight();
                        new_volume += here[i].volume();
                    }
                    getitem[i] = true;
                }
                if (count == here.size()) {
                    for (size_t i = 0; i < here.size(); i++) {
                        getitem[i] = false;
                    }
                    new_weight = g->u.weight_carried();
                    new_volume = g->u.volume_carried();
                }
                update = true;
            }

            for (cur_it = start; cur_it < start + maxitems; cur_it++) {
                mvwprintw(w_pickup, 1 + (cur_it % maxitems), 0,
                          "                                        ");
                if (cur_it < here.size()) {
                    nc_color icolor = here[cur_it].color(&g->u);
                    if (cur_it == selected) {
                        icolor = hilite(icolor);
                    }

                    if (cur_it < pickup_chars.size() ) {
                        mvwputch(w_pickup, 1 + (cur_it % maxitems), 0, icolor,
                                 char(pickup_chars[cur_it]));
                    } else {
                        int p = cur_it - pickup_chars.size();
                        int p1 = p / pickup_chars.size();
                        int p2 = p % pickup_chars.size();
                        mvwprintz(w_pickup, 1 + (cur_it % maxitems), 0, icolor, "`%c%c",
                                  char(pickup_chars[p1]), char(pickup_chars[p2]));
                    }
                    if (getitem[cur_it]) {
                        if (pickup_count[cur_it] == 0) {
                            wprintz(w_pickup, c_ltblue, " + ");
                        } else {
                            wprintz(w_pickup, c_ltblue, " # ");
                        }
                    } else {
                        wprintw(w_pickup, " - ");
                    }
                    wprintz(w_pickup, icolor, "%s", here[cur_it].display_name().c_str());
                }
            }

            int pw = pickupW;
            const char *unmark = _("[left] Unmark");
            const char *scroll = _("[up/dn] Scroll");
            const char *mark   = _("[right] Mark");
            mvwprintw(w_pickup, maxitems + 1, 0,                         unmark);
            mvwprintw(w_pickup, maxitems + 1, (pw - strlen(scroll)) / 2, scroll);
            mvwprintw(w_pickup, maxitems + 1,  pw - strlen(mark),        mark);
            const char *prev = _("[pgup] Prev");
            const char *all = _("[,] All");
            const char *next   = _("[pgdn] Next");
            if (start > 0) {
                mvwprintw(w_pickup, maxitems + 2, 0, prev);
            }
            mvwprintw(w_pickup, maxitems + 2, (pw - strlen(all)) / 2, all);
            if (cur_it < here.size()) {
                mvwprintw(w_pickup, maxitems + 2, pw - strlen(next), next);
            }

            if (update) { // Update weight & volume information
                update = false;
                for (int i = 9; i < pickupW; ++i) {
                    mvwaddch(w_pickup, 0, i, ' ');
                }
                mvwprintz(w_pickup, 0,  9,
                          (new_weight >= g->u.weight_capacity() ? c_red : c_white),
                          _("Wgt %.1f"), g->u.convert_weight(new_weight));
                wprintz(w_pickup, c_white, "/%.1f", g->u.convert_weight(g->u.weight_capacity()));
                mvwprintz(w_pickup, 0, 24,
                          (new_volume > g->u.volume_capacity() - 2 ? c_red : c_white),
                          _("Vol %d"), new_volume);
                wprintz(w_pickup, c_white, "/%d", g->u.volume_capacity() - 2);
            }
            wrefresh(w_pickup);

            ch = (int)getch();

        } while (ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);

        if (ch != '\n') {
            werase(w_pickup);
            wrefresh(w_pickup);
            werase(w_item_info);
            wrefresh(w_item_info);
            delwin(w_pickup);
            delwin(w_item_info);
            add_msg(_("Never mind."));
            g->reenter_fullscreen();
            g->refresh_all();
            return;
        }
    }

    // At this point we've selected our items, now we add them to our inventory
    int curmit = 0;
    bool got_water = false; // Did we try to pick up water?
    bool offered_swap = false;
    std::map<std::string, int> mapPickup;
    for (size_t i = 0; i < here.size(); i++) {
        if (getitem[i] && here[i].made_of(LIQUID)) {
            got_water = true;
        } else if (getitem[i]) {
            bool picked_up = false;
            item temp = here[i].clone();
            int moves_taken = 100;

            here[i].invlet = g->u.inv.get_invlet_for_item( here[i].typeId() );

            if(pickup_count[i] != 0) {
                // Reinserting leftovers happens after item removal to avoid stacking issues.
                int leftover_charges = here[i].charges - pickup_count[i];
                if (leftover_charges > 0) {
                    temp.charges = leftover_charges;
                    here[i].charges = pickup_count[i];
                }
            }

            if (!g->u.can_pickWeight(here[i].weight(), false)) {
                add_msg(m_info, _("The %s is too heavy!"), here[i].display_name().c_str());
            } else if (!g->u.can_pickVolume(here[i].volume())) { //check if player is at max volume
                //try to store arrows/bolts in a worn quiver
                if (here[i].is_ammo() && (here[i].ammo_type() == "arrow" ||
                                          here[i].ammo_type() == "bolt")) {
                    int quivered = handle_quiver_insertion(here[i], false, moves_taken, picked_up);

                    if (here[i].charges > 0) {
                        if(quivered > 0) {
                            //update the charges for the item that gets re-added to the game map
                            pickup_count[i] = quivered;
                            temp.charges = here[i].charges;
                        }

                        add_msg(m_info, ngettext("There's no room in your inventory for the %s.",
                                                 "There's no room in your inventory for the %ss.",
                                                 here[i].charges), here[i].name.c_str());
                    }
                } else if (g->u.is_armed()) {
                    if (!g->u.weapon.has_flag("NO_UNWIELD")) {
                        // Armor can be instantly worn
                        if (here[i].is_armor() &&
                            query_yn(_("Put on the %s?"),
                                     here[i].display_name().c_str())) {
                            if (g->u.wear_item(&(here[i]))) {
                                picked_up = true;
                            }
                        } else if (!offered_swap) {
                            if (query_yn(_("Drop your %s and pick up %s?"),
                                         g->u.weapon.display_name().c_str(),
                                         here[i].display_name().c_str())) {
                                picked_up = true;
                                g->m.add_item_or_charges(posx, posy, g->u.remove_weapon(), 1);
                                g->u.inv.assign_empty_invlet(here[i], true);  // force getting an invlet.
                                g->u.wield(&(g->u.i_add(here[i])));
                                mapPickup[here[i].tname()] += (here[i].count_by_charges()) ?
                                    here[i].charges : 1;
                                add_msg(m_info, _("Wielding %c - %s"), g->u.weapon.invlet,
                                        g->u.weapon.display_name().c_str());
                            }
                            offered_swap = true;
                        }
                    } else {
                        add_msg(m_info, _("There's no room in your inventory for the %s,"
                                          " and you can't unwield your %s."),
                                here[i].display_name().c_str(),
                                g->u.weapon.display_name().c_str());
                    }
                } else {
                    g->u.inv.assign_empty_invlet(here[i], true);  // force getting an invlet.
                    g->u.wield(&(g->u.i_add(here[i])));
                    mapPickup[here[i].tname()] += (here[i].count_by_charges()) ? here[i].charges : 1;
                    picked_up = true;
                }
            } else if (here[i].is_ammo() && (here[i].ammo_type() == "arrow" ||
                                             here[i].ammo_type() == "bolt")) {
                //add ammo to quiver
                handle_quiver_insertion(here[i], true, moves_taken, picked_up);
            } else if (!g->u.is_armed() &&
                       (g->u.volume_carried() + here[i].volume() > g->u.volume_capacity() - 2 ||
                        here[i].is_weap() || here[i].is_gun())) {
                g->u.weapon = here[i];
                picked_up = true;
            } else {
                g->u.i_add(here[i]);
                mapPickup[here[i].tname()] += (here[i].count_by_charges()) ? here[i].charges : 1;
                picked_up = true;
            }

            if (picked_up) {
                pickup_obj.remove_from_map_or_vehicle(posx, posy, veh, cargo_part, moves_taken, curmit);
                curmit--;
                if( pickup_count[i] != 0 ) {
                    bool to_map = !pickup_obj.from_veh;

                    if (pickup_obj.from_veh) {
                        to_map = !veh->add_item( cargo_part, temp );
                    }
                    if (to_map) {
                        g->m.add_item_or_charges( posx, posy, temp );
                    }
                }
            }
        }
        curmit++;
    }

    // Auto pickup item message
    // FIXME: i18n
    if (min == -1 && !mapPickup.empty()) {
        show_pickup_message(mapPickup);
    }

    if (got_water) {
        add_msg(m_info, _("You can't pick up a liquid!"));
    }
    if (weight_is_okay && g->u.weight_carried() >= g->u.weight_capacity()) {
        add_msg(m_bad, _("You're overburdened!"));
    }
    if (volume_is_okay && g->u.volume_carried() > g->u.volume_capacity() - 2) {
        add_msg(m_bad, _("You struggle to carry such a large volume!"));
    }
    g->reenter_fullscreen();
    werase(w_pickup);
    wrefresh(w_pickup);
    werase(w_item_info);
    wrefresh(w_item_info);
    delwin(w_pickup);
    delwin(w_item_info);
}

//helper function for Pickup::pick_up
//return value is amount of ammo added to quiver
int Pickup::handle_quiver_insertion(item &here, bool inv_on_fail, int &moves_to_decrement, bool &picked_up)
{
    //add ammo to quiver
    int quivered = here.add_ammo_to_quiver(&g->u, true);
    if(quivered > 0) {
        moves_to_decrement = 0; //moves already decremented in item::add_ammo_to_quiver()
        picked_up = true;
        return quivered;
    } else if (inv_on_fail) {
        //add to inventory instead
        g->u.i_add(here);
        picked_up = true;

        //display output message
        std::map<std::string, int> map_pickup;
        int charges = (here.count_by_charges()) ? here.charges : 1;
        map_pickup.insert(std::pair<std::string, int>(here.tname(), charges));
        show_pickup_message(map_pickup);
    }
    return 0;
}

//helper function for Pickup::pick_up (singular item)
void Pickup::remove_from_map_or_vehicle(int posx, int posy, vehicle *veh, int cargo_part,
                                int &moves_taken, int curmit)
{
    if (pickup_obj.from_veh) {
        veh->remove_item (cargo_part, curmit);
    } else {
        g->m.i_rem(posx, posy, curmit);
    }
    g->u.moves -= moves_taken;
}

//helper function for Pickup::pick_up
void Pickup::show_pickup_message(std::map<std::string, int> mapPickup)
{
    for (std::map<std::string, int>::iterator iter = mapPickup.begin();
         iter != mapPickup.end(); ++iter) {
        add_msg(ngettext("You pick up: %d %s", "You pick up: %d %ss", iter->second),
                iter->second, iter->first.c_str());
    }
}

