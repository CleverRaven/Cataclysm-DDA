#include "pickup.h"

#include "auto_pickup.h"
#include "game.h"
#include "messages.h"

#include <map>
#include <vector>

// Handles interactions with a vehicle in the examine menu.
// Returns the part number that will accept items if any, or -1 to indicate no cargo part.
// Returns -2 if a special interaction was performed and the menu should exit.
int Pickup::interact_with_vehicle( vehicle *veh, int posx, int posy, int veh_root_part )
{
    bool from_vehicle = false;

    int k_part = 0;
    int wtr_part = 0;
    int w_part = 0;
    int craft_part = 0;
    int cargo_part = 0;
    int chempart = 0;
    int ctrl_part = 0;
    std::vector<std::string> menu_items;
    std::vector<uimenu_entry> options_message;

    auto here_ground = g->m.i_at(posx, posy);
    if( veh ) {
        k_part = veh->part_with_feature(veh_root_part, "KITCHEN");
        wtr_part = veh->part_with_feature(veh_root_part, "FAUCET");
        w_part = veh->part_with_feature(veh_root_part, "WELDRIG");
        craft_part = veh->part_with_feature(veh_root_part, "CRAFTRIG");
        chempart = veh->part_with_feature(veh_root_part, "CHEMLAB");
        cargo_part = veh->part_with_feature(veh_root_part, "CARGO", false);
        ctrl_part = veh->part_with_feature(veh_root_part, "CONTROLS");
        from_vehicle = veh && cargo_part >= 0 && !veh->get_items(cargo_part).empty();

        menu_items.push_back(_("Examine vehicle"));
        options_message.push_back(uimenu_entry(_("Examine vehicle"), 'e'));
        if (ctrl_part >= 0) {
            menu_items.push_back(_("Control vehicle"));
            options_message.push_back(uimenu_entry(_("Control vehicle"), 'v'));
        }

        if( from_vehicle ) {
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
            return -2;
        }
        if(menu_items[choice] == _("Use the hotplate")) {
            //Will be -1 if no battery at all
            item tmp_hotplate( "hotplate", 0 );
            // Drain a ton of power
            tmp_hotplate.charges = veh->drain( "battery", 100 );
            if( tmp_hotplate.is_tool() ) {
                it_tool *tmptool = dynamic_cast<it_tool *>((&tmp_hotplate)->type);
                if ( tmp_hotplate.charges >= tmptool->charges_per_use ) {
                    tmptool->invoke(&g->u, &tmp_hotplate, false, point(posx, posy));
                    tmp_hotplate.charges -= tmptool->charges_per_use;
                    veh->refill( "battery", tmp_hotplate.charges );
                }
            }
            return -2;
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
            return -2;
        }

        if(menu_items[choice] == _("Have a drink")) {
            veh->drain("water", 1);
            item water( "water_clean", 0 );
            g->u.eat(&water, dynamic_cast<it_comest *>(water.type));
            g->u.moves -= 250;
            return -2;
        }

        if(menu_items[choice] == _("Use the welding rig?")) {
            //Will be -1 if no battery at all
            item tmp_welder( "welder", 0 );
            // Drain a ton of power
            tmp_welder.charges = veh->drain( "battery", 1000 );
            if( tmp_welder.is_tool() ) {
                it_tool *tmptool = dynamic_cast<it_tool *>((&tmp_welder)->type);
                if ( tmp_welder.charges >= tmptool->charges_per_use ) {
                    tmptool->invoke( &g->u, &tmp_welder, false, point(posx, posy)  );
                    tmp_welder.charges -= tmptool->charges_per_use;
                    veh->refill( "battery", tmp_welder.charges );
                }
            }
            return -2;
        }

        if(menu_items[choice] == _("Use the water purifier?")) {
            //Will be -1 if no battery at all
            item tmp_purifier( "water_purifier", 0 );
            // Drain a ton of power
            tmp_purifier.charges = veh->drain( "battery", 100 );
            if( tmp_purifier.is_tool() ) {
                it_tool *tmptool = dynamic_cast<it_tool *>((&tmp_purifier)->type);
                if ( tmp_purifier.charges >= tmptool->charges_per_use ) {
                    tmptool->invoke( &g->u, &tmp_purifier, false, point(posx, posy) );
                    tmp_purifier.charges -= tmptool->charges_per_use;
                    veh->refill( "battery", tmp_purifier.charges );
                }
            }
            return -2;
        }

        if(menu_items[choice] == _("Control vehicle") && veh->interact_vehicle_locked()) {
            veh->use_controls();
            return -2;
        }

        if(menu_items[choice] == _("Examine vehicle")) {
            g->exam_vehicle(*veh, posx, posy);
            return -2;
        }

        if(menu_items[choice] == _("Get items on the ground")) {
            from_vehicle = false;
        }
    }
    return from_vehicle ? cargo_part : -1;
}

static bool select_autopickup_items( std::vector<item> &here, std::vector<bool> &getitem )
{
    bool bFoundSomething = false;

    //Loop through Items lowest Volume first
    bool bPickup = false;

    for(size_t iVol = 0, iNumChecked = 0; iNumChecked < here.size(); iVol++) {
        for (size_t i = 0; i < here.size(); i++) {
            bPickup = false;
            if (here[i].volume() == (int)iVol) {
                iNumChecked++;

                //Auto Pickup all items with 0 Volume and Weight <= AUTO_PICKUP_ZERO * 50
                if (OPTIONS["AUTO_PICKUP_ZERO"]) {
                    if (here[i].volume() == 0 &&
                        here[i].weight() <= OPTIONS["AUTO_PICKUP_ZERO"] * 50 &&
                        checkExcludeRules(here[i].tname())) {
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
    return bFoundSomething;
}

void Pickup::pick_one_up( const point &pickup_target, item &newit, vehicle *veh,
                          int cargo_part, int index, int quantity, bool &got_water,
                          bool &offered_swap, std::map<std::string, int> &mapPickup,
                          std::map<std::string, item> &item_info, bool autopickup )
{
    int moves_taken = 100;
    bool picked_up = false;
    item leftovers = newit;

    if( newit.invlet != '\0' &&
        g->u.invlet_to_position( newit.invlet ) != INT_MIN ) {
        // Existing invlet is not re-usable, remove it and let the code in player.cpp/inventory.cpp
        // add a new invlet, otherwise keep the (usable) invlet.
        newit.invlet = '\0';
    }

    if( quantity != 0 ) {
        // Reinserting leftovers happens after item removal to avoid stacking issues.
        int leftover_charges = newit.charges - quantity;
        if (leftover_charges > 0) {
            leftovers.charges = leftover_charges;
            newit.charges = quantity;
        }
    }

    if( newit.made_of(LIQUID) ) {
        got_water = true;
    } else if (!g->u.can_pickWeight(newit.weight(), false)) {
        add_msg(m_info, _("The %s is too heavy!"), newit.display_name().c_str());
    } else if (!g->u.can_pickVolume(newit.volume())) {
        if (newit.is_ammo() && (newit.ammo_type() == "arrow" || newit.ammo_type() == "bolt")) {
            int quivered = handle_quiver_insertion(newit, false, moves_taken, picked_up);
            if (newit.charges > 0) {
                if(quivered > 0) {
                    //update the charges for the item that gets re-added to the game map
                    quantity = quivered;
                    leftovers.charges = newit.charges;
                }
                if( !autopickup ) {
                    // Silence some messaging if we're doing autopickup.
                    add_msg(m_info, ngettext("There's no room in your inventory for the %s.",
                                             "There's no room in your inventory for the %s.",
                                             newit.charges), newit.tname(newit.charges).c_str());
                }
            }
        } else if( !autopickup ) {
            // Armor can be instantly worn
            if (newit.is_armor() &&
                query_yn(_("Put on the %s?"),
                         newit.display_name().c_str())) {
                if (g->u.wear_item(&newit)) {
                    picked_up = true;
                }
            } else if (g->u.is_armed()) {
                if (!g->u.weapon.has_flag("NO_UNWIELD")) {
                     if( !offered_swap ) {
                        offered_swap = true;
                        if ( g->u.weapon.type->id != newit.type->id &&
                             query_yn(_("No space for %s; wield instead? (drops %s)"),
                                      newit.display_name().c_str(),
                                      g->u.weapon.display_name().c_str()) ) {
                            picked_up = true;
                            g->m.add_item_or_charges( pickup_target.x, pickup_target.y,
                                                      g->u.remove_weapon(), 1 );
                            g->u.inv.assign_empty_invlet( newit, true ); // force getting an invlet.
                            g->u.wield( &( g->u.i_add(newit) ) );

                            if (newit.invlet) {
                                add_msg(m_info, _("Wielding %c - %s"), newit.invlet,
                                    newit.display_name().c_str());
                            } else {
                                add_msg(m_info, _("Wielding - %s"), newit.display_name().c_str());
                            }
                        }
                    }
                } else {
                    add_msg(m_info, _("There's no room in your inventory for the %s "
                                      "and you can't unwield your %s."),
                            newit.display_name().c_str(),
                            g->u.weapon.display_name().c_str());
                }
            } else if( !g->u.is_armed()  ) {
                if (g->u.keep_hands_free) {
                    add_msg(m_info, _("There's no room in your inventory for the %s "
                                      "and you have decided to keep your hands free."),
                            newit.display_name().c_str());
                } else {
                    g->u.inv.assign_empty_invlet(newit, true);  // force getting an invlet.
                    g->u.wield(&(g->u.i_add(newit)));
                    picked_up = true;

                    if (newit.invlet) {
                        add_msg(m_info, _("Wielding %c - %s"), newit.invlet,
                            newit.display_name().c_str());
                    } else {
                        add_msg(m_info, _("Wielding - %s"), newit.display_name().c_str());
                    }
                }
            } // end of if unarmed
        } // end of if !autopickup
    } else if (newit.is_ammo() && (newit.ammo_type() == "arrow" || newit.ammo_type() == "bolt")) {
        //add ammo to quiver
        handle_quiver_insertion(newit, true, moves_taken, picked_up);
    } else {
        mapPickup[newit.tname()] += (newit.count_by_charges()) ? newit.charges : 1;
        item_info[newit.tname()] = g->u.i_add(newit);
        picked_up = true;
    }

    if(picked_up) {
        Pickup::remove_from_map_or_vehicle(pickup_target.x, pickup_target.y,
                                           veh, cargo_part, moves_taken, index);
    }
    if( quantity != 0 ) {
        bool to_map = veh == nullptr;
        if( !to_map ) {
            to_map = !veh->add_item( cargo_part, leftovers );
        }
        if( to_map ){
            g->m.add_item_or_charges( pickup_target.x, pickup_target.y, leftovers );
        }
    }
}

void Pickup::do_pickup( point pickup_target, bool from_vehicle,
                        std::list<int> &indices, std::list<int> &quantities, bool autopickup )
{
    bool got_water = false;
    int cargo_part = -1;
    vehicle *veh = nullptr;
    bool weight_is_okay = (g->u.weight_carried() <= g->u.weight_capacity());
    bool volume_is_okay = (g->u.volume_carried() <= g->u.volume_capacity() -  2);
    bool offered_swap = false;
    // Convert from player-relative to map-relative.
    pickup_target.x += g->u.xpos();
    pickup_target.y += g->u.ypos();
    // Map of items picked up so we can output them all at the end and
    // merge dropping items with the same name.
    std::map<std::string, int> mapPickup;
    std::map<std::string, item> item_info;

    if( from_vehicle ) {
        int veh_root_part = -1;
        veh = g->m.veh_at( pickup_target.x, pickup_target.y, veh_root_part );
        cargo_part = veh->part_with_feature( veh_root_part, "CARGO", false );
    }

    while( g->u.moves >= 0 && !indices.empty() ) {
        // Pulling from the back of the (in-order) list of indices insures
        // that we pull from the end of the vector.
        int index = indices.back();
        int quantity = quantities.back();
        // Whether we pick the item up or not, we're done trying to do so,
        // so remove it from the list.
        indices.pop_back();
        quantities.pop_back();

        item *target = nullptr;
        if( from_vehicle ) {
            target = g->m.item_from( veh, cargo_part, index );
        } else {
            target = g->m.item_from( pickup_target, index );
        }

        if( target == nullptr ) {
            continue; // No such item.
        }

        pick_one_up( pickup_target, *target, veh, cargo_part, index, quantity,
                     got_water, offered_swap, mapPickup, item_info, autopickup );
    }

    if( !mapPickup.empty() ) {
        show_pickup_message(mapPickup, item_info);
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
}

// Pick up items at (posx, posy).
void Pickup::pick_up(int posx, int posy, int min)
{
    int veh_root_part = 0;
    int cargo_part = -1;

    vehicle *veh = g->m.veh_at (posx, posy, veh_root_part);
    bool from_vehicle = false;

    if( min != -1 ) {
        cargo_part = interact_with_vehicle( veh, posx, posy, veh_root_part );
        from_vehicle = cargo_part >= 0;
        if( cargo_part == -2 ) {
            // -2 indicates that we already interacted with the vehicle.
            return;
        }
    }

    if (g->m.has_flag("SEALED", posx, posy)) {
        return;
    }

    //min == -1 is Autopickup
    if (!g->u.can_pickup(min != -1)) { // no message on autopickup (-1)
        return;
    }

    if( !from_vehicle ) {
        bool isEmpty = (g->m.i_at(posx, posy).empty());

        // Hide the pickup window if this is a toilet and there's nothing here
        // but water.
        if ((!isEmpty) && g->m.furn(posx, posy) == f_toilet) {
            isEmpty = true;
            for( auto maybe_water : g->m.i_at(posx, posy) ) {
                if( maybe_water.typeId() != "water") {
                    isEmpty = false;
                    break;
                }
            }
        }

        if (isEmpty && (min != -1 || !OPTIONS["AUTO_PICKUP_ADJACENT"] )) {
            return;
        }
    }

    // which items are we grabbing?
    std::vector<item> here;
    if( from_vehicle ) {
        auto vehitems = veh->get_items(cargo_part);
        here.resize( vehitems.size() );
        std::copy( vehitems.begin(), vehitems.end(), here.begin() );
    } else {
        auto mapitems = g->m.i_at(posx, posy);
        here.resize( mapitems.size() );
        std::copy( mapitems.begin(), mapitems.end(), here.begin() );
    }

    if (min == -1) {
        if (g->checkZone("NO_AUTO_PICKUP", posx, posy)) {
            here.clear();
        }

        // Recursively pick up adjacent items if that option is on.
        if( OPTIONS["AUTO_PICKUP_ADJACENT"] && g->u.posx == posx && g->u.posy == posy ) {
            //Autopickup adjacent
            direction adjacentDir[8] = {NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST, NORTHWEST};
            for( auto &elem : adjacentDir ) {

                point apos = direction_XY( elem );
                apos.x += posx;
                apos.y += posy;

                if( g->m.has_flag( "SEALED", apos.x, apos.y ) ) {
                    continue;
                }
                if( g->checkZone( "NO_AUTO_PICKUP", apos.x, apos.y ) ) {
                    continue;
                }
                pick_up( apos.x, apos.y, min );
            }
        }
    }

    // Not many items, just grab them
    if ((int)here.size() <= min && min != -1) {
        g->u.assign_activity( ACT_PICKUP, 0 );
        g->u.activity.placement = point( posx - g->u.xpos(), posy - g->u.ypos() );
        g->u.activity.values.push_back( from_vehicle );
        // Only one item means index is 0.
        g->u.activity.values.push_back( 0 );
        // auto-pickup means pick up all.
        g->u.activity.values.push_back( 0 );
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

    int itemcount = 0;
    std::map<int, unsigned int> pickup_count; // Count of how many we'll pick up from each stack

    if (min == -1) { //Auto Pickup, select matching items
        if( !select_autopickup_items( here, getitem) ) {
            // If we didn't find anything, bail out now.
            return;
        }
    } else {
        int pickupH = maxitems + pickupBorderRows;
        int pickupW = getmaxx(g->w_messages);
        int pickupY = VIEW_OFFSET_Y;
        int pickupX = getbegx(g->w_messages);

        int itemsW = pickupW;
        int itemsY = sideStyle ? pickupY + pickupH : TERMY - itemsH;
        int itemsX = pickupX;

        WINDOW *w_pickup    = newwin(pickupH, pickupW, pickupY, pickupX);
        WINDOW *w_item_info = newwin(itemsH,  itemsW,  itemsY,  itemsX);
        WINDOW_PTR w_pickupptr( w_pickup );
        WINDOW_PTR w_item_infoptr( w_item_info );

        int ch = ' ';
        int start = 0, cur_it;
        int new_weight = g->u.weight_carried(), new_volume = g->u.volume_carried();
        bool update = true;
        mvwprintw(w_pickup, 0, 0, _("PICK UP"));
        int selected = 0;
        int last_selected = -1;

        if(g->was_fullscreen) {
            g->draw_ter();
        }
        // Now print the two lists; those on the ground and about to be added to inv
        // Continue until we hit return or space
        do {
            static const std::string pickup_chars =
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:;";
            int idx = -1;
            for (int i = 1; i < pickupH; i++) {
                mvwprintw(w_pickup, i, 0,
                          "                                                ");
            }
            if (ch >= '0' && ch <= '9') {
                ch = (char)ch - '0';
                itemcount *= 10;
                itemcount += ch;
                if( itemcount < 0 ) {
                    itemcount = 0;
                }
            } else if ((ch == '<' || ch == KEY_PPAGE) && start > 0) {
                start -= maxitems;
                selected = start;
                mvwprintw(w_pickup, maxitems + 2, 0, "         ");
            } else if ((ch == '>' || ch == KEY_NPAGE) && start + maxitems < (int)here.size()) {
                start += maxitems;
                selected = start;
                mvwprintw(w_pickup, maxitems + 2, pickupH, "            ");
            } else if ( ch == KEY_UP ) {
                selected--;
                if ( selected < 0 ) {
                    selected = here.size() - 1;
                    start = (int)( here.size() / maxitems ) * maxitems;
                    if (start >= (int)here.size()) {
                        start -= maxitems;
                    }
                } else if ( selected < start ) {
                    start -= maxitems;
                }
            } else if ( ch == KEY_DOWN ) {
                selected++;
                if ( selected >= (int)here.size() ) {
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
                std::string ext = string_input_popup(
                                      _("Enter 2 letters (case sensitive):"), 3, "", "", "", 2);
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

            if( idx >= 0 && idx < (int)here.size()) {
                if( getitem[idx] ) {
                    if( pickup_count[idx] != 0 && (int)pickup_count[idx] < here[idx].charges ) {
                        item temp = here[idx];
                        temp.charges = pickup_count[idx];
                        new_weight -= temp.weight();
                        new_volume -= temp.volume();
                    } else {
                        new_weight -= here[idx].weight();
                        new_volume -= here[idx].volume();
                    }
                }
                if (itemcount != 0 || pickup_count[idx] == 0) {
                    if (itemcount >= here[idx].charges || !here[idx].count_by_charges()) {
                        // Ignore the count if we pickup the whole stack anyway
                        // or something that is not counted by charges (tools)
                        itemcount = 0;
                    }
                    pickup_count[idx] = itemcount;
                    itemcount = 0;
                }

                // Note: this might not change the value of getitem[idx] at all!
                getitem[idx] = ( ch == KEY_RIGHT ? true : ( ch == KEY_LEFT ? false : !getitem[idx] ) );
                if ( ch != KEY_RIGHT && ch != KEY_LEFT) {
                    selected = idx;
                    start = (int)( idx / maxitems ) * maxitems;
                }

                if (getitem[idx]) {
                    if (pickup_count[idx] != 0 &&
                        (int)pickup_count[idx] < here[idx].charges) {
                        item temp = here[idx];
                        temp.charges = pickup_count[idx];
                        new_weight += temp.weight();
                        new_volume += temp.volume();
                    } else {
                        new_weight += here[idx].weight();
                        new_volume += here[idx].volume();
                    }
                } else {
                    pickup_count[idx] = 0;
                }
                update = true;
            }

            if ( selected != last_selected ) {
                last_selected = selected;
                werase(w_item_info);
                if ( selected >= 0 && selected <= (int)here.size() - 1 ) {
                    std::vector<iteminfo> vThisItem, vDummy;
                    here[selected].info(true, &vThisItem);

                    draw_item_info(w_item_info, "", vThisItem, vDummy, 0, true, true);
                }
                draw_border(w_item_info);
                const auto name = utf8_wrapper( here[selected].display_name() ).shorten( itemsW - 8 );
                mvwprintz(w_item_info, 0, 2, c_white, "< %s >", name.c_str());
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
                if (count == (int)here.size()) {
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
                if (cur_it < (int)here.size()) {
                    nc_color icolor = here[cur_it].color(&g->u);
                    if (cur_it == selected) {
                        icolor = hilite(icolor);
                    }

                    if (cur_it < (int)pickup_chars.size() ) {
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
            if (cur_it < (int)here.size()) {
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

        bool item_selected = false;
        // Check if we have selected an item.
        for( auto selection : getitem ) {
            if( selection ) {
                item_selected = true;
            }
        }
        if( ch != '\n' || !item_selected ) {
            w_pickupptr.reset();
            w_item_infoptr.reset();
            add_msg(_("Never mind."));
            g->reenter_fullscreen();
            g->refresh_all();
            return;
        }
    }

    // At this point we've selected our items, register an activity to pick them up.
    g->u.assign_activity( ACT_PICKUP, 0 );
    g->u.activity.placement = point( posx - g->u.xpos(), posy - g->u.ypos() );
    g->u.activity.values.push_back( from_vehicle );
    if( min == -1 ) {
        // Auto pickup will need to auto resume since there can be several of them on the stack.
        g->u.activity.auto_resume = true;
    }
    for (size_t i = 0; i < here.size(); i++) {
        if( getitem[i] ) {
            g->u.activity.values.push_back( i );
            g->u.activity.values.push_back( pickup_count[i] );
        }
    }

    g->reenter_fullscreen();
}

//helper function for Pickup::pick_up
//return value is amount of ammo added to quiver
int Pickup::handle_quiver_insertion(item &here, bool inv_on_fail, int &moves_to_decrement,
                                    bool &picked_up)
{
    //add ammo to quiver
    int quivered = here.add_ammo_to_quiver(&g->u, true);
    if(quivered > 0) {
        moves_to_decrement = 0; //moves already decremented in item::add_ammo_to_quiver()
        picked_up = true;
        return quivered;
    } else if (inv_on_fail) {
        //add to inventory instead
        item& it = g->u.i_add(here);
        picked_up = true;

        //display output message
        std::map<std::string, int> map_pickup;
        std::map<std::string, item> item_info;
        int charges = (here.count_by_charges()) ? here.charges : 1;
        map_pickup.insert(std::pair<std::string, int>(here.tname(), charges));
        item_info.insert(std::pair<std::string, item>(here.tname(), it));
        show_pickup_message(map_pickup, item_info);
    }
    return 0;
}

//helper function for Pickup::pick_up (singular item)
void Pickup::remove_from_map_or_vehicle(int posx, int posy, vehicle *veh, int cargo_part,
                                        int &moves_taken, int curmit)
{
    if( veh != nullptr ) {
        veh->remove_item( cargo_part, curmit );
    } else {
        g->m.i_rem( posx, posy, curmit );
    }
    g->u.moves -= moves_taken;
}

//helper function for Pickup::pick_up
void Pickup::show_pickup_message(std::map<std::string, int> &mapPickup,
                                 std::map<std::string, item> &item_info)
{
    // iterator _should_ be the same, as std::map is ordered...
    auto mp_iter = mapPickup.begin();
    auto ii_iter = item_info.begin();

    while(mp_iter != mapPickup.end() && ii_iter != item_info.end()) {
        // name seems to be a fitting test
        if(mp_iter->first == ii_iter->first) {
            int  const  quantity   = mp_iter->second;
            std::string name       = ii_iter->second.display_name(quantity);
            char const  letter     = ii_iter->second.invlet;
            bool const  use_letter = letter != 0;

            if (use_letter) {
                add_msg(_("You pick up: %d %s [%c]"), quantity, name.c_str(), letter);
            } else {
                add_msg(_("You pick up: %d %s"), quantity, name.c_str());
            }
        } else {
            // ... and if it for some reason isn't, catch it in debug logs.
            debugmsg("show_pickup_message: mp_iter->first [%s] != ii_iter->first [%s]",
                    mp_iter->first.c_str(), ii_iter->first.c_str());
        }

        ++mp_iter;
        ++ii_iter;
    }
}
