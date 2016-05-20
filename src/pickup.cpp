#include "pickup.h"

#include "auto_pickup.h"
#include "game.h"
#include "player.h"
#include "map.h"
#include "messages.h"
#include "translations.h"
#include "input.h"
#include "output.h"
#include "options.h"
#include "ui.h"
#include "itype.h"
#include "vehicle.h"
#include "mapdata.h"
#include "cata_utility.h"
#include "debug.h"

#include <map>
#include <vector>
#include <cstring>

struct pickup_count {
    bool pick = false;
    int count = 0;
    explicit operator bool() const {
        return pick;
    }
};

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

    if( has_faucet && veh->fuel_left("water_clean") > 0) {
        selectmenu.addentry( FILL_CONTAINER, true, 'c', _("Fill a container with water") );

        selectmenu.addentry( DRINK, true, 'd', _("Have a drink") );
    }

    if( has_weldrig && veh->fuel_left("battery") > 0 ) {
        selectmenu.addentry( USE_WELDER, true, 'w', _("Use the welding rig?") );
    }

    if( has_purify && veh->fuel_left("battery") > 0 ) {
        selectmenu.addentry( USE_PURIFIER, true, 'p', _("Purify water in carried container") );
    }

    if( has_purify && veh->fuel_left("battery") > 0 &&
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
        item pseudo( "hotplate" );
        itype_id ammo = pseudo.ammo_default();
        pseudo.ammo_set( ammo, veh->drain( ammo, pseudo.ammo_capacity() ) );

        if ( pseudo.ammo_remaining() >= pseudo.ammo_required() ) {
            g->u.invoke_item( &pseudo );
            pseudo.ammo_consume( pseudo.ammo_required(), g->u.pos() );
            veh->refill( ammo, pseudo.ammo_remaining() );
        }
        return DONE;
    }

    case FILL_CONTAINER:
        g->u.siphon( *veh, "water_clean" );
        return DONE;

    case DRINK: {
        veh->drain("water_clean", 1);
        item water( "water_clean", 0 );
        g->u.eat( water );
        g->u.moves -= 250;
        return DONE;
        }

    case USE_WELDER: {
        item pseudo( "welder" );
        itype_id ammo = pseudo.ammo_default();
        pseudo.ammo_set( ammo, veh->drain( ammo, pseudo.ammo_capacity() ) );

        if ( pseudo.ammo_remaining() >= pseudo.ammo_required() ) {
            g->u.invoke_item( &pseudo );
            pseudo.ammo_consume( pseudo.ammo_required(), g->u.pos() );
            veh->refill( ammo, pseudo.ammo_remaining() );

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
        return DONE;
    }

    case USE_PURIFIER: {
        item pseudo( "water_purifier" );
        itype_id ammo = pseudo.ammo_default();
        pseudo.ammo_set( ammo, veh->drain( ammo, pseudo.ammo_capacity() ) );

        if ( pseudo.ammo_remaining() >= pseudo.ammo_required() ) {
            g->u.invoke_item( &pseudo );
            pseudo.ammo_consume( pseudo.ammo_required(), g->u.pos() );
            veh->refill( ammo, pseudo.ammo_remaining() );
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

static bool select_autopickup_items( std::vector<item> &here, std::vector<pickup_count> &getitem )
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
                getitem[i].pick = bPickup;
                bFoundSomething = true;
            }
        }
    }
    return bFoundSomething;
}

enum pickup_answer :int {
    CANCEL = -1,
    WIELD,
    WEAR,
    SPILL,
    STASH,
    NUM_ANSWERS
};

pickup_answer handle_problematic_pickup( const item &it, bool &offered_swap, const std::string &explain )
{
    player &u = g->u;
    if( !u.is_armed() && !u.keep_hands_free ) {
        return WIELD;
    }

    uimenu amenu;
    amenu.return_invalid = true;

    amenu.selected = 0;
    amenu.text = explain;

    offered_swap = true;
    // @todo Gray out if not enough hands
    amenu.addentry( WIELD, !u.weapon.has_flag( "NO_UNWIELD" ), 'w',
                    _("Dispose of %s and wield %s"), u.weapon.display_name().c_str(),
                    it.display_name().c_str() );
    if( it.is_armor() ) {
        // @todo Gray out for mutants?
        amenu.addentry( WEAR, u.can_wear( it ), 'W', _("Wear %s"), it.display_name().c_str() );
    }
    if( !it.is_container_empty() && u.can_pickVolume( it.volume() ) ) {
        amenu.addentry( SPILL, true, 's', _("Spill %s, then pick up %s"),
                        it.contents[0].tname().c_str(), it.display_name().c_str() );
    }

    amenu.query();
    int choice = amenu.ret;

    if( choice <= CANCEL || choice >= NUM_ANSWERS ) {
        return CANCEL;
    }

    return static_cast<pickup_answer>( choice );
}

void Pickup::pick_one_up( const tripoint &pickup_target, item &newit, vehicle *veh,
                          int cargo_part, int index, int quantity, bool &got_water,
                          bool &offered_swap, PickupMap &mapPickup, bool autopickup )
{
    player &u = g->u;
    int moves_taken = 100;
    bool picked_up = false;
    pickup_answer option = CANCEL;
    item leftovers = newit;

    if( newit.invlet != '\0' &&
        u.invlet_to_position( newit.invlet ) != INT_MIN ) {
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
    } else if (!u.can_pickWeight(newit.weight(), false)) {
        add_msg(m_info, _("The %s is too heavy!"), newit.display_name().c_str());
    } else if( newit.is_ammo() && (newit.ammo_type() == "arrow" || newit.ammo_type() == "bolt")) {
        // @todo Make quiver code generic so that ammo pouches can use it too
        //add ammo to quiver
        int quivered = handle_quiver_insertion( newit, moves_taken, picked_up );

        if( quivered > 0 ) {
            quantity = quivered;
            //already picked up some for quiver so use special case handling
            picked_up = true;
            option = NUM_ANSWERS;
        }
        if( newit.charges > 0 ) {
            if( !u.can_pickVolume( newit.volume() ) ) {
                if( !autopickup ) {
                    // Silence some messaging if we're doing autopickup.
                    add_msg(m_info, ngettext("There's no room in your inventory for the %s.",
                                             "There's no room in your inventory for the %s.",
                                             newit.charges), newit.tname(newit.charges).c_str());
                }
            } else {
                // Add to inventory instead
                option = STASH;
            }
        }
        if( option == NUM_ANSWERS ) {
            //not picking up the rest so
            //update the charges for the item that gets re-added to the game map
            leftovers.charges = newit.charges;
        }
    } else if( newit.is_bucket() && !newit.is_container_empty() ) {
        if( !autopickup ) {
            const std::string &explain = string_format( _("Can't stash %s while it's not empty"),
                                                        newit.display_name().c_str() );
            option = handle_problematic_pickup( newit, offered_swap, explain );
        } else {
            option = CANCEL;
        }
    } else if( !u.can_pickVolume( newit.volume() ) ) {
        if( !autopickup ) {
            const std::string &explain = string_format( _("Not enough capacity to stash %s"),
                                                        newit.display_name().c_str() );
            option = handle_problematic_pickup( newit, offered_swap, explain );
        } else {
            option = CANCEL;
        }
    } else {
        option = STASH;
    }

    switch( option ) {
        case NUM_ANSWERS:
            // Some other option
            break;
        case CANCEL:
            picked_up = false;
            break;
        case WEAR:
            picked_up = u.wear_item( newit );
            break;
        case WIELD:
            picked_up = u.wield( newit );
            if( !picked_up ) {
                break;
            }

            if( u.weapon.invlet ) {
                add_msg( m_info, _("Wielding %c - %s"), u.weapon.invlet,
                         u.weapon.display_name().c_str() );
            } else {
                add_msg( m_info, _("Wielding - %s"), u.weapon.display_name().c_str() );
            }
            break;
        case SPILL:
            if( newit.is_container_empty() ) {
                debugmsg( "Tried to spill contents from an empty container" );
                break;
            }

            picked_up = newit.spill_contents( u );
            if( !picked_up ) {
                break;
            }
            // Intentional fallthrough
        case STASH:
            auto &entry = mapPickup[newit.tname()];
            entry.second += newit.count_by_charges() ? newit.charges : 1;
            entry.first = u.i_add( newit );
            picked_up = true;
            break;
    }

    if( picked_up ) {
        Pickup::remove_from_map_or_vehicle( pickup_target, veh, cargo_part, moves_taken, index );
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
            if (g->m.has_flag("SEALED", pos)) {
                return;
            }

            break;
        }
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
        std::copy( vehitems.rbegin(), vehitems.rend(), here.begin() );
    } else {
        auto mapitems = g->m.i_at(pos);
        here.resize( mapitems.size() );
        std::copy( mapitems.rbegin(), mapitems.rend(), here.begin() );
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

    std::vector<pickup_count> getitem( here.size() );

    int maxitems = here.size();
    maxitems = (maxitems < minmaxitems ? minmaxitems : (maxitems > maxmaxitems ? maxmaxitems :
                maxitems ));

    int itemcount = 0;

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

        std::string action;
        long raw_input_char = ' ';
        input_context ctxt("PICKUP");
        ctxt.register_action("UP");
        ctxt.register_action("DOWN");
        ctxt.register_action("RIGHT");
        ctxt.register_action("LEFT");
        ctxt.register_action("NEXT_TAB", _("Next page"));
        ctxt.register_action("PREV_TAB", _("Previous page"));
        ctxt.register_action("SCROLL_UP");
        ctxt.register_action("SCROLL_DOWN");
        ctxt.register_action("CONFIRM");
        ctxt.register_action("SELECT_ALL");
        ctxt.register_action("QUIT", _("Cancel"));
        ctxt.register_action("ANY_INPUT");
        ctxt.register_action("HELP_KEYBINDINGS");

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
            const std::string pickup_chars = ctxt.get_available_single_char_hotkeys(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:;");
            int idx = -1;
            for (int i = 1; i < pickupH; i++) {
                mvwprintw(w_pickup, i, 0,
                          "                                                ");
            }
            if ( action == "ANY_INPUT" &&
                    raw_input_char >= '0' && raw_input_char <= '9') {
                int raw_input_char_value = (char)raw_input_char - '0';
                itemcount *= 10;
                itemcount += raw_input_char_value;
                if( itemcount < 0 ) {
                    itemcount = 0;
                }
            } else if ( action == "SCROLL_UP") {
                iScrollPos--;
            } else if ( action == "SCROLL_DOWN") {
                iScrollPos++;
            } else if ( action == "PREV_TAB" ) {
                if ( start > 0 ) {
                    start -= maxitems;
                } else {
                    start = (int)( (here.size()-1) / maxitems ) * maxitems;
                }
                selected = start;
                mvwprintw(w_pickup, maxitems + 2, 0, "         ");
            } else if ( action == "NEXT_TAB" ) {
                if ( start + maxitems < (int)here.size() ) {
                    start += maxitems;
                } else {
                    start = 0;
                }
                iScrollPos = 0;
                selected = start;
                mvwprintw(w_pickup, maxitems + 2, pickupH, "            ");
            } else if ( action == "UP" ) {
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
            } else if ( action == "DOWN" ) {
                selected++;
                iScrollPos = 0;
                if ( selected >= (int)here.size() ) {
                    selected = 0;
                    start = 0;
                } else if ( selected >= start + maxitems ) {
                    start += maxitems;
                }
            } else if ( selected >= 0 && (
                            ( action == "RIGHT" && !getitem[selected]) ||
                            ( action == "LEFT" && getitem[selected] )
                        ) ) {
                idx = selected;
            } else if ( action == "ANY_INPUT" && raw_input_char == '`' ) {
                std::string ext = string_input_popup(
                                      _("Enter 2 letters (case sensitive):"), 3, "", "", "", 2);
                if(ext.size() == 2) {
                    int p1 = pickup_chars.find(ext.at(0));
                    int p2 = pickup_chars.find(ext.at(1));
                    if ( p1 != -1 && p2 != -1 ) {
                        idx = pickup_chars.size() + ( p1 * pickup_chars.size() ) + p2;
                    }
                }
            } else if ( action == "ANY_INPUT" ) {
                idx = ( raw_input_char <= 127 ) ? pickup_chars.find(raw_input_char) : -1;
                iScrollPos = 0;
            }

            if( idx >= 0 && idx < (int)here.size()) {
                if( getitem[idx] ) {
                    if( getitem[idx].count != 0 && getitem[idx].count < here[idx].charges ) {
                        item temp = here[idx];
                        temp.charges = getitem[idx].count;
                        new_weight -= temp.weight();
                        new_volume -= temp.volume();
                    } else {
                        new_weight -= here[idx].weight();
                        new_volume -= here[idx].volume();
                    }
                }
                if (itemcount != 0 || getitem[idx].count == 0) {
                    if (itemcount >= here[idx].charges || !here[idx].count_by_charges()) {
                        // Ignore the count if we pickup the whole stack anyway
                        // or something that is not counted by charges (tools)
                        itemcount = 0;
                    }
                    getitem[idx].count = itemcount;
                    itemcount = 0;
                }

                // Note: this might not change the value of getitem[idx] at all!
                getitem[idx].pick = ( action == "RIGHT" ? true : ( action == "LEFT" ? false : !getitem[idx] ) );
                if ( action != "RIGHT" && action != "LEFT" ) {
                    selected = idx;
                    start = (int)( idx / maxitems ) * maxitems;
                }

                if (getitem[idx]) {
                    if (getitem[idx].count != 0 &&
                        getitem[idx].count < here[idx].charges) {
                        item temp = here[idx];
                        temp.charges = getitem[idx].count;
                        new_weight += temp.weight();
                        new_volume += temp.volume();
                    } else {
                        new_weight += here[idx].weight();
                        new_volume += here[idx].volume();
                    }
                } else {
                    getitem[idx].count = 0;
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

            if (action == "SELECT_ALL") {
                int count = 0;
                for (size_t i = 0; i < here.size(); i++) {
                    if (getitem[i]) {
                        count++;
                    } else {
                        new_weight += here[i].weight();
                        new_volume += here[i].volume();
                    }
                    getitem[i].pick = true;
                }
                if (count == (int)here.size()) {
                    for (size_t i = 0; i < here.size(); i++) {
                        getitem[i].pick = false;
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
                    } else if ( cur_it < (int)pickup_chars.size() + (int)pickup_chars.size() * (int)pickup_chars.size() ) {
                        int p = cur_it - pickup_chars.size();
                        int p1 = p / pickup_chars.size();
                        int p2 = p % pickup_chars.size();
                        mvwprintz(w_pickup, 1 + (cur_it % maxitems), 0, icolor, "`%c%c",
                                  char(pickup_chars[p1]), char(pickup_chars[p2]));
                    } else {
                        mvwputch(w_pickup, 1 + (cur_it % maxitems), 0, icolor, ' ');
                    }
                    if (getitem[cur_it]) {
                        if (getitem[cur_it].count == 0) {
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

            mvwprintw( w_pickup, maxitems + 1, 0, _( "[%s] Unmark" ),
                       ctxt.get_desc( "LEFT", 1 ).c_str() );

            center_print( w_pickup, maxitems + 1, c_ltgray, _( "[%s] Help" ),
                          ctxt.get_desc( "HELP_KEYBINDINGS", 1 ).c_str() );

            right_print( w_pickup, maxitems + 1, 0, c_ltgray, _( "[%s] Mark" ),
                         ctxt.get_desc( "RIGHT", 1 ).c_str() );

            mvwprintw( w_pickup, maxitems + 2, 0, _( "[%s] Prev" ),
                       ctxt.get_desc( "PREV_TAB", 1 ).c_str() );

            center_print( w_pickup, maxitems + 2, c_ltgray, _( "[%s] All" ),
                          ctxt.get_desc( "SELECT_ALL", 1 ).c_str() );

            right_print( w_pickup, maxitems + 2, 0, c_ltgray, _( "[%s] Next" ),
                         ctxt.get_desc( "NEXT_TAB", 1 ).c_str() );

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

            action = ctxt.handle_input();
            raw_input_char = ctxt.get_raw_input().get_first_input();

        } while (action != "QUIT" && action != "CONFIRM");

        bool item_selected = false;
        // Check if we have selected an item.
        for( auto selection : getitem ) {
            if( selection ) {
                item_selected = true;
            }
        }
        if( action != "CONFIRM" || !item_selected ) {
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
    std::reverse( getitem.begin(), getitem.end() );
    for( size_t i = 0; i < here.size(); i++ ) {
        if( getitem[i] ) {
            g->u.activity.values.push_back( i );
            g->u.activity.values.push_back( getitem[i].count );
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
