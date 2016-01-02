#include "pickup.h"

#include "auto_pickup.h"
#include "game.h"
#include "player.h"
#include "map.h"
#include "messages.h"
#include "translations.h"
#include "input.h"
#include "options.h"
#include "ui.h"
#include "itype.h"
#include "vehicle.h"
#include "mapdata.h"
#include "cata_utility.h"

#include <map>
#include <vector>
#include <cstring>

// Handles interactions with a vehicle in the examine menu.
Pickup::interact_results Pickup::interact_with_vehicle( vehicle *veh, const tripoint &pos, int veh_root_part )
{
    if( veh == nullptr ) {
        return ITEMS_FROM_GROUND;
    }

    std::vector<std::string> menu_items;
    std::vector<uimenu_entry> options_message;
    const bool has_items_on_ground = g->m.sees_some_items( pos, g->u );
    const bool items_are_sealed = g->m.has_flag( "SEALED", pos );

    const bool has_kitchen = (veh->part_with_feature(veh_root_part, "KITCHEN") >= 0);
    const bool has_faucet = (veh->part_with_feature(veh_root_part, "FAUCET") >= 0);
    const bool has_weldrig = (veh->part_with_feature(veh_root_part, "WELDRIG") >= 0);
    const bool has_craftrig = (veh->part_with_feature(veh_root_part, "CRAFTRIG") >= 0);
    const bool has_chemlab = (veh->part_with_feature(veh_root_part, "CHEMLAB") >= 0);
    const bool has_purify = (veh->part_with_feature(veh_root_part, "WATER_PURIFIER") >=0);
    const bool has_controls = ((veh->part_with_feature(veh_root_part, "CONTROLS") >= 0) ||
                               (veh->part_with_feature(veh_root_part, "CTRL_ELECTRONIC") >=0));
    const int cargo_part = veh->part_with_feature(veh_root_part, "CARGO", false);
    const bool from_vehicle = veh && cargo_part >= 0 && !veh->get_items(cargo_part).empty();
    const bool can_be_folded = veh->is_foldable();
    const bool is_convertible = (veh->tags.count("convertible") > 0);
    const bool remotely_controlled = g->remoteveh() == veh;
    typedef enum {
        EXAMINE, CONTROL, GET_ITEMS, GET_ITEMS_ON_GROUND, FOLD_VEHICLE, USE_HOTPLATE,
        FILL_CONTAINER, DRINK, USE_WELDER, USE_PURIFIER, PURIFY_TANK,
    } options;
    uimenu selectmenu;

    selectmenu.addentry( EXAMINE, true, 'e', _("Examine vehicle") );

    if( has_controls ) {
        selectmenu.addentry( CONTROL, true, 'v', _("Control vehicle") );
    }

    if( from_vehicle ) {
        selectmenu.addentry( GET_ITEMS, true, 'g', _("Get items") );
    }

    if( has_items_on_ground && !items_are_sealed ) {
        selectmenu.addentry( GET_ITEMS_ON_GROUND, true, 'i', _("Get items on the ground") );
    }

    if( ( can_be_folded || is_convertible ) && !remotely_controlled ) {
        selectmenu.addentry( FOLD_VEHICLE, true, 'f', _("Fold vehicle") );
    }

    if( ( has_kitchen || has_chemlab ) && veh->fuel_left("battery") > 0) {
        selectmenu.addentry( USE_HOTPLATE, true, 'h', _("Use the hotplate") );
    }

    if( ( has_kitchen || has_faucet ) && veh->fuel_left("water_clean") > 0) {
        selectmenu.addentry( FILL_CONTAINER, true, 'c', _("Fill a container with water") );

        selectmenu.addentry( DRINK, true, 'd', _("Have a drink") );
    }

    if( has_weldrig && veh->fuel_left("battery") > 0 ) {
        selectmenu.addentry( USE_WELDER, true, 'w', _("Use the welding rig?") );
    }

    if( ( has_craftrig || has_purify ) && veh->fuel_left("battery") > 0 ) {
        selectmenu.addentry( USE_PURIFIER, true, 'p', _("Purify water in carried container") );
    }

    if( ( has_craftrig || has_purify ) && veh->fuel_left("battery") > 0 &&
        veh->fuel_left("water") > 0 &&
        veh->fuel_capacity("water_clean") > veh->fuel_left("water_clean") ) {
        selectmenu.addentry( PURIFY_TANK, true, 'P', _("Purify water in vehicle's tank") );
    }

    int choice;
    if( selectmenu.entries.size() == 1 ) {
        choice = selectmenu.entries.front().retval;
    } else {
        selectmenu.return_invalid = true;
        selectmenu.text = _("Select an action");
        selectmenu.selected = 0;
        selectmenu.query();
        choice = selectmenu.ret;
    }

    switch( static_cast<options>( choice ) ) {
    case USE_HOTPLATE: {
        //Will be -1 if no battery at all
        item tmp_hotplate( "hotplate", 0 );
        // Drain a ton of power
        tmp_hotplate.charges = veh->drain( "battery", 100 );
        if( tmp_hotplate.is_tool() ) {
            const auto tmptool = dynamic_cast<const it_tool *>((&tmp_hotplate)->type);
            if ( tmp_hotplate.charges >= tmptool->charges_per_use ) {
                g->u.invoke_item( &tmp_hotplate );
                tmp_hotplate.charges -= tmptool->charges_per_use;
                veh->refill( "battery", tmp_hotplate.charges );
            }
        }
        return DONE;
        }

    case FILL_CONTAINER: {
        int amt = veh->drain("water_clean", veh->fuel_left("water_clean"));
        item fill_water( "water_clean", calendar::turn );
        fill_water.charges = amt;
        int back = g->move_liquid(fill_water);
        if (back >= 0) {
            veh->refill("water_clean", back);
        } else {
            veh->refill("water_clean", amt);
        }
        return DONE;
        }

    case DRINK: {
        veh->drain("water_clean", 1);
        item water( "water_clean", 0 );
        g->u.eat(&water, dynamic_cast<const it_comest *>(water.type));
        g->u.moves -= 250;
        return DONE;
        }

    case USE_WELDER: {
        //Will be -1 if no battery at all
        item tmp_welder( "welder", 0 );
        // Drain a ton of power
        tmp_welder.charges = veh->drain( "battery", 1000 );
        if( tmp_welder.is_tool() ) {
            const auto tmptool = dynamic_cast<const it_tool *>((&tmp_welder)->type);
            if ( tmp_welder.charges >= tmptool->charges_per_use ) {
                g->u.invoke_item( &tmp_welder );
                tmp_welder.charges -= tmptool->charges_per_use;
                veh->refill( "battery", tmp_welder.charges );
                // Evil hack incoming
                auto &act = g->u.activity;
                if( act.type == ACT_REPAIR_ITEM ) {
                    // Magic: first tell activity the item doesn't really exist
                    act.index = INT_MIN;
                    // Then tell it to search it on `pos`
                    act.coords.push_back( pos );
                    // Finally tell it it is the vehicle part with weldrig
                    act.values.resize( 2 );
                    act.values[1] = veh->part_with_feature( veh_root_part, "WELDRIG" );
                }
            }
        }
        return DONE;
        }

    case USE_PURIFIER: {
        //Will be -1 if no battery at all
        item tmp_purifier( "water_purifier", 0 );
        // Drain a ton of power
        tmp_purifier.charges = veh->drain( "battery", veh->fuel_left("battery"));
        if( tmp_purifier.is_tool() ) {
            const auto tmptool = dynamic_cast<const it_tool *>((&tmp_purifier)->type);
            if ( tmp_purifier.charges >= tmptool->charges_per_use ) {
                g->u.invoke_item( &tmp_purifier );
                tmp_purifier.charges -= tmptool->charges_per_use;
                veh->refill( "battery", tmp_purifier.charges );
            }
        }
        return DONE;
        }

    case PURIFY_TANK: {
        const int max_water = std::min( veh->fuel_left("water"),
            veh->fuel_capacity("water_clean") - veh->fuel_left("water_clean") );
        const int purify_amount = std::min( veh->fuel_left("battery"), max_water );
        veh->drain( "battery", purify_amount );
        veh->drain( "water", purify_amount );
        veh->refill( "water_clean", purify_amount );
        return DONE;
        }

    case FOLD_VEHICLE:
        veh->fold_up();
        return DONE;

    case CONTROL:
        if( veh->interact_vehicle_locked() ) {
            veh->use_controls(pos);
        }
        return DONE;

    case EXAMINE:
        g->exam_vehicle(*veh, pos );
        return DONE;

    case GET_ITEMS_ON_GROUND:
        return ITEMS_FROM_GROUND;

    case GET_ITEMS:
        return from_vehicle ? ITEMS_FROM_CARGO : ITEMS_FROM_GROUND;
    }

    return DONE;
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
                const std::string sItemName = here[i].tname( 1, false );

                //Check the Pickup Rules
                if ( get_auto_pickup().check_item(sItemName) == "true" ) {
                    bPickup = true;
                } else if ( get_auto_pickup().check_item(sItemName) != "false" ) {
                    //No prematched pickup rule found
                    //items with damage, (fits) or a container
                    get_auto_pickup().create_rules(sItemName);

                    if ( get_auto_pickup().check_item(sItemName) == "true" ) {
                        bPickup = true;
                    }
                }

                //Auto Pickup all items with 0 Volume and Weight <= AUTO_PICKUP_ZERO * 50
                //items will either be in the autopickup list ("true") or unmatched ("")
                if (!bPickup && OPTIONS["AUTO_PICKUP_ZERO"]) {
                    if (here[i].volume() == 0 &&
                        here[i].weight() <= OPTIONS["AUTO_PICKUP_ZERO"] * 50 &&
                        get_auto_pickup().check_item(sItemName) != "false") {
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

void Pickup::pick_one_up( const tripoint &pickup_target, item &newit, vehicle *veh,
                          int cargo_part, int index, int quantity, bool &got_water,
                          bool &offered_swap, PickupMap &mapPickup, bool autopickup )
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

    if( quantity != 0 && newit.count_by_charges() ) {
        // Reinserting leftovers happens after item removal to avoid stacking issues.
        leftovers.charges = newit.charges - quantity;
        if( leftovers.charges > 0 ) {
            newit.charges = quantity;
        }
    } else {
        leftovers.charges = 0;
    }

    if( newit.made_of(LIQUID) ) {
        got_water = true;
    } else if (!g->u.can_pickWeight(newit.weight(), false)) {
        add_msg(m_info, _("The %s is too heavy!"), newit.display_name().c_str());
    } else if( newit.is_ammo() && (newit.ammo_type() == "arrow" || newit.ammo_type() == "bolt")) {
        //add ammo to quiver
        int quivered = handle_quiver_insertion( newit, moves_taken, picked_up);
        if( newit.charges > 0) {
            if(!g->u.can_pickVolume( newit.volume())) {
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
            } else {
                //add to inventory instead
                item &it = g->u.i_add(newit);
                picked_up = true;

                //display output message
                PickupMap map_pickup;
                int charges = (newit.count_by_charges()) ? newit.charges : 1;
                map_pickup.insert(std::pair<std::string, ItemCount>(newit.tname(), ItemCount(it, charges)));
                show_pickup_message(map_pickup);
            }
        }
    } else if (!g->u.can_pickVolume(newit.volume())) {
        if( !autopickup ) {
            // Armor can be instantly worn
            if (newit.is_armor() &&
                query_yn(_("Put on the %s?"),
                         newit.display_name().c_str())) {
                if (g->u.wear_item(newit)) {
                    picked_up = true;
                }
            } else if (g->u.is_armed()) {
                if (!g->u.weapon.has_flag("NO_UNWIELD")) {
                    if( !offered_swap ) {
                        offered_swap = true;
                        if ( g->u.weapon.type->id != newit.type->id &&
                             query_yn(_("No space for %1$s; wield instead? (drops %2$s)"),
                                      newit.display_name().c_str(),
                                      g->u.weapon.display_name().c_str()) ) {
                            picked_up = true;
                            g->m.add_item_or_charges( pickup_target,
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
    } else {
        auto &entry = mapPickup[newit.tname()];
        entry.second += newit.count_by_charges() ? newit.charges : 1;
        entry.first = g->u.i_add(newit);
        picked_up = true;
    }

    if(picked_up) {
        Pickup::remove_from_map_or_vehicle(pickup_target,
                                           veh, cargo_part, moves_taken, index);
    }
    if( leftovers.charges > 0 ) {
        bool to_map = veh == nullptr;
        if( !to_map ) {
            to_map = !veh->add_item( cargo_part, leftovers );
        }
        if( to_map ) {
            g->m.add_item_or_charges( pickup_target, leftovers );
        }
    }
}

void Pickup::do_pickup( const tripoint &pickup_target_arg, bool from_vehicle,
                        std::list<int> &indices, std::list<int> &quantities, bool autopickup )
{
    bool got_water = false;
    int cargo_part = -1;
    vehicle *veh = nullptr;
    bool weight_is_okay = (g->u.weight_carried() <= g->u.weight_capacity());
    bool volume_is_okay = (g->u.volume_carried() <= g->u.volume_capacity());
    bool offered_swap = false;
    // Convert from player-relative to map-relative.
    tripoint pickup_target = pickup_target_arg + g->u.pos();
    // Map of items picked up so we can output them all at the end and
    // merge dropping items with the same name.
    PickupMap mapPickup;

    if( from_vehicle ) {
        int veh_root_part = -1;
        veh = g->m.veh_at( pickup_target, veh_root_part );
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
                     got_water, offered_swap, mapPickup, autopickup );
    }

    if( !mapPickup.empty() ) {
        show_pickup_message(mapPickup);
    }

    if (got_water) {
        add_msg(m_info, _("You can't pick up a liquid!"));
    }
    if (weight_is_okay && g->u.weight_carried() > g->u.weight_capacity()) {
        add_msg(m_bad, _("You're overburdened!"));
    }
    if (volume_is_okay && g->u.volume_carried() > g->u.volume_capacity()) {
        add_msg(m_bad, _("You struggle to carry such a large volume!"));
    }
}

// Pick up items at (pos).
void Pickup::pick_up( const tripoint &pos, int min )
{
    int veh_root_part = 0;
    int cargo_part = -1;

    vehicle *veh = g->m.veh_at (pos, veh_root_part);
    bool from_vehicle = false;

    if( min != -1 ) {
        switch( interact_with_vehicle( veh, pos, veh_root_part ) ) {
        case DONE:
            return;
        case ITEMS_FROM_CARGO:
            cargo_part = veh->part_with_feature( veh_root_part, "CARGO", false );
            from_vehicle = cargo_part >= 0;
            break;
        case ITEMS_FROM_GROUND:
            // Nothing to change, default is to pick from ground anyway.
            break;
        }
    }

    if (g->m.has_flag("SEALED", pos)) {
        return;
    }

    if( !from_vehicle ) {
        bool isEmpty = (g->m.i_at(pos).empty());

        // Hide the pickup window if this is a toilet and there's nothing here
        // but water.
        if ((!isEmpty) && g->m.furn(pos) == f_toilet) {
            isEmpty = true;
            for( auto maybe_water : g->m.i_at(pos) ) {
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
        auto mapitems = g->m.i_at(pos);
        here.resize( mapitems.size() );
        std::copy( mapitems.begin(), mapitems.end(), here.begin() );
    }

    if (min == -1) {
        if( g->check_zone( "NO_AUTO_PICKUP", pos ) ) {
            here.clear();
        }

        // Recursively pick up adjacent items if that option is on.
        if( OPTIONS["AUTO_PICKUP_ADJACENT"] && g->u.pos() == pos ) {
            //Autopickup adjacent
            direction adjacentDir[8] = {NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST, NORTHWEST};
            for( auto &elem : adjacentDir ) {

                tripoint apos = tripoint( direction_XY( elem ), 0 );
                apos += pos;

                if( g->m.has_flag( "SEALED", apos ) ) {
                    continue;
                }
                if( g->check_zone( "NO_AUTO_PICKUP", apos ) ) {
                    continue;
                }
                pick_up( apos, min );
            }
        }
    }

    // Not many items, just grab them
    if ((int)here.size() <= min && min != -1) {
        g->u.assign_activity( ACT_PICKUP, 0 );
        g->u.activity.placement = pos - g->u.pos();
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
        int iScrollPos = 0;

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
            } else if ( ch == KEY_PPAGE) {
                iScrollPos--;
            } else if ( ch == KEY_NPAGE) {
                iScrollPos++;
            } else if ( ch == '<' ) {
                if ( start > 0 ) {
                    start -= maxitems;
                } else {
                    start = (int)( (here.size()-1) / maxitems ) * maxitems;
                }
                selected = start;
                mvwprintw(w_pickup, maxitems + 2, 0, "         ");
            } else if ( ch == '>' ) {
                if ( start + maxitems < (int)here.size() ) {
                    start += maxitems;
                } else {
                    start = 0;
                }
                iScrollPos = 0;
                selected = start;
                mvwprintw(w_pickup, maxitems + 2, pickupH, "            ");
            } else if ( ch == KEY_UP ) {
                selected--;
                iScrollPos = 0;
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
                iScrollPos = 0;
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
                idx = ( ch <= 127 ) ? pickup_chars.find(ch) : -1;
                iScrollPos = 0;
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

            werase(w_item_info);
            if ( selected >= 0 && selected <= (int)here.size() - 1 ) {
                std::vector<iteminfo> vThisItem, vDummy;
                here[selected].info(true, vThisItem);

                draw_item_info(w_item_info, "", "", vThisItem, vDummy, iScrollPos, true, true);
            }
            draw_custom_border(w_item_info, false);
            mvwprintw(w_item_info, 0, 2, "< ");
            trim_and_print(w_item_info, 0, 4, itemsW - 8, c_white, "%s >", here[selected].display_name().c_str());
            wrefresh(w_item_info);

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
                    nc_color icolor = here[cur_it].color_in_inventory();
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
                    std::string item_name = here[cur_it].display_name();
                    if (OPTIONS["ITEM_SYMBOLS"]) {
                        item_name = string_format("%c %s", here[cur_it].symbol(), item_name.c_str());
                    }
                    trim_and_print(w_pickup, 1 + (cur_it % maxitems), 6, pickupW - 4, icolor,
                                   "%s", item_name.c_str());
                }
            }

            int pw = pickupW;
            const char *unmark = _("[left] Unmark");
            const char *scroll = _("[up/dn] Scroll");
            const char *mark   = _("[right] Mark");
            mvwprintw(w_pickup, maxitems + 1, 0,                         unmark);
            mvwprintw(w_pickup, maxitems + 1, (pw - std::strlen(scroll)) / 2, scroll);
            mvwprintw(w_pickup, maxitems + 1,  pw - std::strlen(mark),        mark);
            const char *prev = _("[<] Prev");
            const char *all = _("[,] All");
            const char *next   = _("[>] Next");
            mvwprintw(w_pickup, maxitems + 2, 0, prev);
            mvwprintw(w_pickup, maxitems + 2, (pw - std::strlen(all)) / 2, all);
            mvwprintw(w_pickup, maxitems + 2, pw - std::strlen(next), next);

            if (update) { // Update weight & volume information
                update = false;
                for (int i = 9; i < pickupW; ++i) {
                    mvwaddch(w_pickup, 0, i, ' ');
                }
                mvwprintz(w_pickup, 0,  9,
                          (new_weight > g->u.weight_capacity() ? c_red : c_white),
                          _("Wgt %.1f"), convert_weight(new_weight) + 0.05); // +0.05 to round up
                wprintz(w_pickup, c_white, "/%.1f", convert_weight(g->u.weight_capacity()));
                mvwprintz(w_pickup, 0, 24,
                          (new_volume > g->u.volume_capacity() ? c_red : c_white),
                          _("Vol %d"), new_volume);
                wprintz(w_pickup, c_white, "/%d", g->u.volume_capacity());
            }
            wrefresh(w_pickup);

            ch = (int)getch();

        } while (ch != ' ' && ch != '\n' && ch != KEY_ENTER && ch != KEY_ESCAPE);

        bool item_selected = false;
        // Check if we have selected an item.
        for( auto selection : getitem ) {
            if( selection ) {
                item_selected = true;
            }
        }
        if( (ch != '\n' && ch != KEY_ENTER) || !item_selected ) {
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
    g->u.activity.placement = pos - g->u.pos();
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
int Pickup::handle_quiver_insertion(item &here, int &moves_to_decrement, bool &picked_up)
{
    //add ammo to quiver
    int quivered = g->u.add_ammo_to_worn_quiver( here );
    if( quivered > 0 ) {
        moves_to_decrement = 0; //moves already decremented in player::add_ammo_to_worn_quiver()
        picked_up = true;
        return quivered;
    }

    return 0;
}

//helper function for Pickup::pick_up (singular item)
void Pickup::remove_from_map_or_vehicle( const tripoint &pos, vehicle *veh, int cargo_part,
                                        int &moves_taken, int curmit )
{
    if( veh != nullptr ) {
        veh->remove_item( cargo_part, curmit );
    } else {
        g->m.i_rem( pos, curmit );
    }
    g->u.moves -= moves_taken;
}

//helper function for Pickup::pick_up
void Pickup::show_pickup_message( const PickupMap &mapPickup )
{
    for( auto &entry : mapPickup ) {
            if( entry.second.first.invlet != 0 ) {
                add_msg(_("You pick up: %d %s [%c]"), entry.second.second,
                        entry.second.first.display_name(entry.second.second).c_str(), entry.second.first.invlet);
            } else {
                add_msg(_("You pick up: %d %s"), entry.second.second,
                        entry.second.first.display_name(entry.second.second).c_str());
            }
    }
}
