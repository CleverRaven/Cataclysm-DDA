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
#include "vpart_position.h"
#include "vehicle.h"
#include "vpart_reference.h"
#include "mapdata.h"
#include "cata_utility.h"
#include "string_formatter.h"
#include "debug.h"
#include "vehicle_selector.h"
#include "veh_interact.h"
#include "item_search.h"
#include "item_location.h"
#include "string_input_popup.h"

#include <map>
#include <vector>
#include <string>
#include <cstring>

typedef std::pair<item, int> ItemCount;
typedef std::map<std::string, ItemCount> PickupMap;

// Pickup helper functions
static bool pick_one_up( const tripoint &pickup_target, item &newit,
                         vehicle *veh, int cargo_part, int index, int quantity,
                         bool &got_water, bool &offered_swap,
                         PickupMap &mapPickup, bool autopickup );

typedef enum {
    DONE, ITEMS_FROM_CARGO, ITEMS_FROM_GROUND,
} interact_results;

static interact_results interact_with_vehicle( vehicle *veh, const tripoint &vpos,
        int veh_root_part );

static void remove_from_map_or_vehicle( const tripoint &pos, vehicle *veh, int cargo_part,
                                        int moves_taken, int curmit );
static void show_pickup_message( const PickupMap &mapPickup );

struct pickup_count {
    bool pick = false;
    //count is 0 if the whole stack is being picked up, nonzero otherwise.
    int count = 0;
};

struct item_idx {
    item _item;
    size_t idx;
};

// Handles interactions with a vehicle in the examine menu.
interact_results interact_with_vehicle( vehicle *veh, const tripoint &pos,
                                        int veh_root_part )
{
    if( veh == nullptr ) {
        return ITEMS_FROM_GROUND;
    }

    std::vector<std::string> menu_items;
    std::vector<uimenu_entry> options_message;
    const bool has_items_on_ground = g->m.sees_some_items( pos, g->u );
    const bool items_are_sealed = g->m.has_flag( "SEALED", pos );

    auto turret = veh->turret_query( pos );

    const bool has_kitchen = ( veh->part_with_feature( veh_root_part, "KITCHEN" ) >= 0 );
    const bool has_faucet = ( veh->part_with_feature( veh_root_part, "FAUCET" ) >= 0 );
    const bool has_weldrig = ( veh->part_with_feature( veh_root_part, "WELDRIG" ) >= 0 );
    const bool has_chemlab = ( veh->part_with_feature( veh_root_part, "CHEMLAB" ) >= 0 );
    const bool has_purify = ( veh->part_with_feature( veh_root_part, "WATER_PURIFIER" ) >= 0 );
    const bool has_controls = ( ( veh->part_with_feature( veh_root_part, "CONTROLS" ) >= 0 ) ||
                                ( veh->part_with_feature( veh_root_part, "CTRL_ELECTRONIC" ) >= 0 ) );
    const int cargo_part = veh->part_with_feature( veh_root_part, "CARGO", false );
    const bool from_vehicle = cargo_part >= 0 && !veh->get_items( cargo_part ).empty();
    const bool can_be_folded = veh->is_foldable();
    const bool is_convertible = ( veh->tags.count( "convertible" ) > 0 );
    const bool remotely_controlled = g->remoteveh() == veh;
    const int washing_machine_part = veh->part_with_feature( veh_root_part, "WASHING_MACHINE" );
    const bool has_washmachine = washing_machine_part >= 0;
    bool washing_machine_on = ( washing_machine_part == -1 ) ? false :
                              veh->parts[washing_machine_part].enabled;
    const bool has_monster_capture = ( veh->part_with_feature( veh_root_part,
                                       "CAPTURE_MONSTER_VEH" ) >= 0 );
    const int monster_capture_part = veh->part_with_feature( veh_root_part, "CAPTURE_MONSTER_VEH" );

    typedef enum {
        EXAMINE, TRACK, CONTROL, CONTROL_ELECTRONICS, GET_ITEMS, GET_ITEMS_ON_GROUND, FOLD_VEHICLE, UNLOAD_TURRET, RELOAD_TURRET,
        USE_HOTPLATE, FILL_CONTAINER, DRINK, USE_WELDER, USE_PURIFIER, PURIFY_TANK, USE_WASHMACHINE, USE_MONSTER_CAPTURE
    } options;
    uimenu selectmenu;

    selectmenu.addentry( EXAMINE, true, 'e', _( "Examine vehicle" ) );
    selectmenu.addentry( TRACK, true, keybind( "TOGGLE_TRACKING" ), veh->tracking_toggle_string() );

    if( has_controls ) {
        selectmenu.addentry( CONTROL, true, 'v', _( "Control vehicle" ) );
        selectmenu.addentry( CONTROL_ELECTRONICS, true, keybind( "CONTROL_MANY_ELECTRONICS" ),
                             _( "Control multiple electronics" ) );
    }

    if( has_washmachine ) {
        selectmenu.addentry( USE_WASHMACHINE, true, 'W',
                             washing_machine_on ? _( "Deactivate the washing machine" ) :
                             _( "Activate the washing machine (1.5 hours)" ) );
    }

    if( from_vehicle && !washing_machine_on ) {
        selectmenu.addentry( GET_ITEMS, true, 'g', _( "Get items" ) );
    }

    if( has_items_on_ground && !items_are_sealed ) {
        selectmenu.addentry( GET_ITEMS_ON_GROUND, true, 'i', _( "Get items on the ground" ) );
    }

    if( ( can_be_folded || is_convertible ) && !remotely_controlled ) {
        selectmenu.addentry( FOLD_VEHICLE, true, 'f', _( "Fold vehicle" ) );
    }

    if( turret.can_unload() ) {
        selectmenu.addentry( UNLOAD_TURRET, true, 'u', _( "Unload %s" ), turret.name().c_str() );
    }

    if( turret.can_reload() ) {
        selectmenu.addentry( RELOAD_TURRET, true, 'r', _( "Reload %s" ), turret.name().c_str() );
    }

    if( ( has_kitchen || has_chemlab ) && veh->fuel_left( "battery" ) > 0 ) {
        selectmenu.addentry( USE_HOTPLATE, true, 'h', _( "Use the hotplate" ) );
    }

    if( has_faucet && veh->fuel_left( "water_clean" ) > 0 ) {
        selectmenu.addentry( FILL_CONTAINER, true, 'c', _( "Fill a container with water" ) );

        selectmenu.addentry( DRINK, true, 'd', _( "Have a drink" ) );
    }

    if( has_weldrig && veh->fuel_left( "battery" ) > 0 ) {
        selectmenu.addentry( USE_WELDER, true, 'w', _( "Use the welding rig?" ) );
    }

    if( has_purify ) {
        bool can_purify = veh->fuel_left( "battery" ) >=
                          item::find_type( "water_purifier" )->charges_to_use();

        selectmenu.addentry( USE_PURIFIER, can_purify,
                             'p', _( "Purify water in carried container" ) );

        selectmenu.addentry( PURIFY_TANK, can_purify && veh->fuel_left( "water" ),
                             'P', _( "Purify water in vehicle tank" ) );
    }
    if( has_monster_capture ) {
        selectmenu.addentry( USE_MONSTER_CAPTURE, true, 'G', _( "Capture or release a creature" ) );
    }

    int choice;
    if( selectmenu.entries.size() == 1 ) {
        choice = selectmenu.entries.front().retval;
    } else {
        selectmenu.return_invalid = true;
        selectmenu.text = _( "Select an action" );
        selectmenu.selected = 0;
        selectmenu.query();
        choice = selectmenu.ret;
    }

    auto veh_tool = [&]( const itype_id & obj ) {
        item pseudo( obj );
        if( veh->fuel_left( "battery" ) < pseudo.ammo_required() ) {
            return false;
        }
        auto qty = pseudo.ammo_capacity() - veh->discharge_battery( pseudo.ammo_capacity() );
        pseudo.ammo_set( "battery", qty );
        g->u.invoke_item( &pseudo );
        veh->charge_battery( pseudo.ammo_remaining() );
        return true;
    };

    switch( static_cast<options>( choice ) ) {
        case USE_MONSTER_CAPTURE: {
            veh->use_monster_capture( monster_capture_part, pos );
            return DONE;
        }

        case USE_HOTPLATE:
            veh_tool( "hotplate" );
            return DONE;

        case USE_WASHMACHINE: {
            veh->use_washing_machine( washing_machine_part );
            return DONE;
        }

        case FILL_CONTAINER:
            g->u.siphon( *veh, "water_clean" );
            return DONE;

        case DRINK: {
            item water( "water_clean", 0 );
            if( g->u.eat( water ) ) {
                veh->drain( "water_clean", 1 );
                g->u.moves -= 250;
            }
            return DONE;
        }

        case USE_WELDER: {
            if( veh_tool( "welder" ) ) {
                // Evil hack incoming
                auto &act = g->u.activity;
                if( act.id() == activity_id( "ACT_REPAIR_ITEM" ) ) {
                    // Magic: first tell activity the item doesn't really exist
                    act.index = INT_MIN;
                    // Then tell it to search it on `pos`
                    act.coords.push_back( pos );
                    // Finally tell if it is the vehicle part with welding rig
                    act.values.resize( 2 );
                    act.values[1] = veh->part_with_feature( veh_root_part, "WELDRIG" );
                }
            }
            return DONE;
        }

        case USE_PURIFIER:
            veh_tool( "water_purifier" );
            return DONE;

        case PURIFY_TANK: {
            auto sel = []( const vehicle_part & pt ) {
                return pt.is_tank() && pt.ammo_current() == "water";
            };

            auto title = string_format( _( "Purify <color_%s>water</color> in tank" ),
                                        get_all_colors().get_name( item::find_type( "water" )->color ).c_str() );

            auto &tank = veh_interact::select_part( *veh, sel, title );

            if( tank ) {
                double cost = item::find_type( "water_purifier" )->charges_to_use();

                if( veh->fuel_left( "battery" ) < tank.ammo_remaining() * cost ) {
                    //~ $1 - vehicle name, $2 - part name
                    add_msg( m_bad, _( "Insufficient power to purify the contents of the %1$s's %2$s" ),
                             veh->name.c_str(), tank.name().c_str() );

                } else {
                    //~ $1 - vehicle name, $2 - part name
                    add_msg( m_good, _( "You purify the contents of the %1$s's %2$s" ),
                             veh->name.c_str(), tank.name().c_str() );

                    veh->discharge_battery( tank.ammo_remaining() * cost );
                    tank.ammo_set( "water_clean", tank.ammo_remaining() );
                }
            }
            return DONE;
        }

        case UNLOAD_TURRET: {
            g->unload( *turret.base() );
            return DONE;
        }

        case RELOAD_TURRET: {
            item::reload_option opt = g->u.select_ammo( *turret.base(), true );
            if( opt ) {
                g->u.assign_activity( activity_id( "ACT_RELOAD" ), opt.moves(), opt.qty() );
                g->u.activity.targets.emplace_back( turret.base() );
                g->u.activity.targets.push_back( std::move( opt.ammo ) );
            }
            return DONE;
        }

        case FOLD_VEHICLE:
            veh->fold_up();
            return DONE;

        case CONTROL:
            veh->use_controls( pos );
            return DONE;

        case CONTROL_ELECTRONICS:
            veh->control_electronics();
            return DONE;

        case EXAMINE:
            g->exam_vehicle( *veh );
            return DONE;

        case TRACK:
            veh->toggle_tracking( );
            return DONE;

        case GET_ITEMS_ON_GROUND:
            return ITEMS_FROM_GROUND;

        case GET_ITEMS:
            return from_vehicle ? ITEMS_FROM_CARGO : ITEMS_FROM_GROUND;
    }

    return DONE;
}

static bool select_autopickup_items( std::vector<std::list<item_idx>> &here,
                                     std::vector<pickup_count> &getitem )
{
    bool bFoundSomething = false;

    //Loop through Items lowest Volume first
    bool bPickup = false;

    for( size_t iVol = 0, iNumChecked = 0; iNumChecked < here.size(); iVol++ ) {
        for( size_t i = 0; i < here.size(); i++ ) {
            bPickup = false;
            if( here[i].begin()->_item.volume() / units::legacy_volume_factor == ( int )iVol ) {
                iNumChecked++;
                const std::string sItemName = here[i].begin()->_item.tname( 1, false );

                //Check the Pickup Rules
                if( get_auto_pickup().check_item( sItemName ) == RULE_WHITELISTED ) {
                    bPickup = true;
                } else if( get_auto_pickup().check_item( sItemName ) != RULE_BLACKLISTED ) {
                    //No prematched pickup rule found
                    //items with damage, (fits) or a container
                    get_auto_pickup().create_rule( sItemName );

                    if( get_auto_pickup().check_item( sItemName ) == RULE_WHITELISTED ) {
                        bPickup = true;
                    }
                }

                //Auto Pickup all items with Volume <= AUTO_PICKUP_VOL_LIMIT * 50 and Weight <= AUTO_PICKUP_ZERO * 50
                //items will either be in the autopickup list ("true") or unmatched ("")
                if( !bPickup ) {
                    int weight_limit = get_option<int>( "AUTO_PICKUP_WEIGHT_LIMIT" );
                    int volume_limit = get_option<int>( "AUTO_PICKUP_VOL_LIMIT" );
                    if( weight_limit && volume_limit ) {
                        if( here[i].begin()->_item.volume() <= units::from_milliliter( volume_limit * 50 ) &&
                            here[i].begin()->_item.weight() <= weight_limit * 50_gram &&
                            get_auto_pickup().check_item( sItemName ) != RULE_BLACKLISTED ) {
                            bPickup = true;
                        }
                    }
                }
            }

            if( bPickup ) {
                getitem[i].pick = true;
                bFoundSomething = true;
            }
        }
    }
    return bFoundSomething;
}

enum pickup_answer : int {
    CANCEL = -1,
    WIELD,
    WEAR,
    SPILL,
    STASH,
    NUM_ANSWERS
};

pickup_answer handle_problematic_pickup( const item &it, bool &offered_swap,
        const std::string &explain )
{
    if( offered_swap ) {
        return CANCEL;
    }

    player &u = g->u;

    uimenu amenu;
    amenu.return_invalid = true;

    amenu.selected = 0;
    amenu.text = explain;

    offered_swap = true;
    // @todo: Gray out if not enough hands
    if( u.is_armed() ) {
        amenu.addentry( WIELD, !u.weapon.has_flag( "NO_UNWIELD" ), 'w',
                        _( "Dispose of %s and wield %s" ), u.weapon.display_name().c_str(),
                        it.display_name().c_str() );
    } else {
        amenu.addentry( WIELD, true, 'w', _( "Wield %s" ), it.display_name().c_str() );
    }
    if( it.is_armor() ) {
        amenu.addentry( WEAR, u.can_wear( it ).success(), 'W', _( "Wear %s" ), it.display_name().c_str() );
    }
    if( it.is_bucket_nonempty() ) {
        amenu.addentry( SPILL, u.can_pickVolume( it ), 's', _( "Spill %s, then pick up %s" ),
                        it.contents.front().tname().c_str(), it.display_name().c_str() );
    }

    amenu.query();
    int choice = amenu.ret;

    if( choice <= CANCEL || choice >= NUM_ANSWERS ) {
        return CANCEL;
    }

    return static_cast<pickup_answer>( choice );
}

// Returns false if pickup caused a prompt and the player selected to cancel pickup
bool pick_one_up( const tripoint &pickup_target, item &newit, vehicle *veh,
                  int cargo_part, int index, int quantity, bool &got_water,
                  bool &offered_swap, PickupMap &mapPickup, bool autopickup )
{
    player &u = g->u;
    int moves_taken = 100;
    bool picked_up = false;
    pickup_answer option = CANCEL;
    item leftovers = newit;
    const auto wield_check = u.can_wield( newit );

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

    bool did_prompt = false;
    newit.charges = u.i_add_to_container( newit, false );
    if( newit.is_ammo() && newit.charges == 0 ) {
        picked_up = true;
        option = NUM_ANSWERS; //Skip the options part
    } else if( newit.made_of( LIQUID ) ) {
        got_water = true;
    } else if( !u.can_pickWeight( newit, false ) ) {
        if( !autopickup ) {
            const std::string &explain = string_format( _( "The %s is too heavy!" ),
                                         newit.display_name().c_str() );
            option = handle_problematic_pickup( newit, offered_swap, explain );
            did_prompt = true;
        } else {
            option = CANCEL;
        }
    } else if( newit.is_bucket() && !newit.is_container_empty() ) {
        if( !autopickup ) {
            const std::string &explain = string_format( _( "Can't stash %s while it's not empty" ),
                                         newit.display_name().c_str() );
            option = handle_problematic_pickup( newit, offered_swap, explain );
            did_prompt = true;
        } else {
            option = CANCEL;
        }
    } else if( !u.can_pickVolume( newit ) ) {
        if( !autopickup ) {
            const std::string &explain = string_format( _( "Not enough capacity to stash %s" ),
                                         newit.display_name().c_str() );
            option = handle_problematic_pickup( newit, offered_swap, explain );
            did_prompt = true;
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
            if( wield_check.success() ) {
                picked_up = u.wield( newit );
                if( u.weapon.invlet ) {
                    add_msg( m_info, _( "Wielding %c - %s" ), u.weapon.invlet,
                             u.weapon.display_name().c_str() );
                } else {
                    add_msg( m_info, _( "Wielding - %s" ), u.weapon.display_name().c_str() );
                }
            } else {
                add_msg( wield_check.c_str() );
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
        remove_from_map_or_vehicle( pickup_target, veh, cargo_part, moves_taken, index );
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

    return picked_up || !did_prompt;
}

bool Pickup::do_pickup( const tripoint &pickup_target_arg, bool from_vehicle,
                        std::list<int> &indices, std::list<int> &quantities, bool autopickup )
{
    bool got_water = false;
    int cargo_part = -1;
    vehicle *veh = nullptr;
    bool weight_is_okay = ( g->u.weight_carried() <= g->u.weight_capacity() );
    bool volume_is_okay = ( g->u.volume_carried() <= g->u.volume_capacity() );
    bool offered_swap = false;
    // Convert from player-relative to map-relative.
    tripoint pickup_target = pickup_target_arg + g->u.pos();
    // Map of items picked up so we can output them all at the end and
    // merge dropping items with the same name.
    PickupMap mapPickup;

    if( from_vehicle ) {
        const cata::optional<vpart_reference> vp = g->m.veh_at( pickup_target ).part_with_feature( "CARGO",
                false );
        veh = &vp->vehicle();
        cargo_part = vp->part_index();
    }

    bool problem = false;
    while( !problem && g->u.moves >= 0 && !indices.empty() ) {
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

        problem = !pick_one_up( pickup_target, *target, veh, cargo_part, index, quantity,
                                got_water, offered_swap, mapPickup, autopickup );
    }

    if( !mapPickup.empty() ) {
        show_pickup_message( mapPickup );
    }

    if( got_water ) {
        add_msg( m_info, _( "You can't pick up a liquid!" ) );
    }
    if( weight_is_okay && g->u.weight_carried() > g->u.weight_capacity() ) {
        add_msg( m_bad, _( "You're overburdened!" ) );
    }
    if( volume_is_okay && g->u.volume_carried() > g->u.volume_capacity() ) {
        add_msg( m_bad, _( "You struggle to carry such a large volume!" ) );
    }

    return !problem;
}

// Pick up items at (pos).
void Pickup::pick_up( const tripoint &pos, int min )
{
    int cargo_part = -1;

    const optional_vpart_position vp = g->m.veh_at( pos );
    vehicle *const veh = veh_pointer_or_null( vp );
    bool from_vehicle = false;

    if( min != -1 ) {
        switch( interact_with_vehicle( veh, pos, vp ? vp->part_index() : -1 ) ) {
            case DONE:
                return;
            case ITEMS_FROM_CARGO: {
                const cata::optional<vpart_reference> carg = vp.part_with_feature( "CARGO", false );
                cargo_part = carg ? carg->part_index() : -1;
            }
            from_vehicle = cargo_part >= 0;
            break;
            case ITEMS_FROM_GROUND:
                // Nothing to change, default is to pick from ground anyway.
                if( g->m.has_flag( "SEALED", pos ) ) {
                    return;
                }

                break;
        }
    }

    if( !from_vehicle ) {
        bool isEmpty = ( g->m.i_at( pos ).empty() );

        // Hide the pickup window if this is a toilet and there's nothing here
        // but water.
        if( ( !isEmpty ) && g->m.furn( pos ) == f_toilet ) {
            isEmpty = true;
            for( auto maybe_water : g->m.i_at( pos ) ) {
                if( maybe_water.typeId() != "water" ) {
                    isEmpty = false;
                    break;
                }
            }
        }

        if( isEmpty && ( min != -1 || !get_option<bool>( "AUTO_PICKUP_ADJACENT" ) ) ) {
            return;
        }
    }

    // which items are we grabbing?
    std::vector<item> here;
    if( from_vehicle ) {
        auto vehitems = veh->get_items( cargo_part );
        here.resize( vehitems.size() );
        std::copy( vehitems.begin(), vehitems.end(), here.begin() );
    } else {
        auto mapitems = g->m.i_at( pos );
        here.resize( mapitems.size() );
        std::copy( mapitems.begin(), mapitems.end(), here.begin() );
    }

    if( min == -1 ) {
        // Recursively pick up adjacent items if that option is on.
        if( get_option<bool>( "AUTO_PICKUP_ADJACENT" ) && g->u.pos() == pos ) {
            //Autopickup adjacent
            direction adjacentDir[8] = {NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST, NORTHWEST};
            for( auto &elem : adjacentDir ) {

                tripoint apos = tripoint( direction_XY( elem ), 0 );
                apos += pos;

                pick_up( apos, min );
            }
        }

        // Bail out if this square cannot be auto-picked-up
        if( g->check_zone( zone_type_id( "NO_AUTO_PICKUP" ), pos ) ) {
            return;
        } else if( g->m.has_flag( "SEALED", pos ) ) {
            return;
        }
    }

    // Not many items, just grab them
    if( ( int )here.size() <= min && min != -1 ) {
        g->u.assign_activity( activity_id( "ACT_PICKUP" ) );
        g->u.activity.placement = pos - g->u.pos();
        g->u.activity.values.push_back( from_vehicle );
        // Only one item means index is 0.
        g->u.activity.values.push_back( 0 );
        // auto-pickup means pick up all.
        g->u.activity.values.push_back( 0 );
        return;
    }

    std::vector<std::list<item_idx>> stacked_here;
    for( size_t i = 0; i < here.size(); i++ ) {
        item &it = here[i];
        bool found_stack = false;
        for( auto &stack : stacked_here ) {
            if( stack.begin()->_item.stacks_with( it ) ) {
                item_idx el = { it, i };
                stack.push_back( el );
                found_stack = true;
                break;
            }
        }
        if( !found_stack ) {
            std::list<item_idx> newstack;
            newstack.push_back( { it, i } );
            stacked_here.push_back( newstack );
        }
    }
    std::reverse( stacked_here.begin(), stacked_here.end() );

    if( min != -1 ) { // don't bother if we're just autopickuping
        g->temp_exit_fullscreen();
    }
    bool sideStyle = use_narrow_sidebar();

    // Otherwise, we have Autopickup, 2 or more items and should list them, etc.
    int maxmaxitems = sideStyle ? TERMY : getmaxy( g->w_messages ) - 3;

    int itemsH = std::min( 25, TERMY / 2 );
    int pickupBorderRows = 3;

    // The pickup list may consume the entire terminal, minus space needed for its
    // header/footer and the item info window.
    int minleftover = itemsH + pickupBorderRows;
    if( maxmaxitems > TERMY - minleftover ) {
        maxmaxitems = TERMY - minleftover;
    }

    const int minmaxitems = sideStyle ? 6 : 9;

    std::vector<pickup_count> getitem( stacked_here.size() );

    int maxitems = stacked_here.size();
    maxitems = ( maxitems < minmaxitems ? minmaxitems : ( maxitems > maxmaxitems ? maxmaxitems :
                 maxitems ) );

    int itemcount = 0;

    if( min == -1 ) { //Auto Pickup, select matching items
        if( !select_autopickup_items( stacked_here, getitem ) ) {
            // If we didn't find anything, bail out now.
            return;
        }
    } else {
        int pickupH = maxitems + pickupBorderRows;
        int pickupW = getmaxx( g->w_messages );
        int pickupY = VIEW_OFFSET_Y;
        int pickupX = getbegx( g->w_messages );

        int itemsW = pickupW;
        int itemsY = sideStyle ? pickupY + pickupH : TERMY - itemsH;
        int itemsX = pickupX;

        catacurses::window w_pickup = catacurses::newwin( pickupH, pickupW, pickupY, pickupX );
        catacurses::window w_item_info = catacurses::newwin( itemsH,  itemsW,  itemsY,  itemsX );

        std::string action;
        long raw_input_char = ' ';
        input_context ctxt( "PICKUP" );
        ctxt.register_action( "UP" );
        ctxt.register_action( "DOWN" );
        ctxt.register_action( "RIGHT" );
        ctxt.register_action( "LEFT" );
        ctxt.register_action( "NEXT_TAB", _( "Next page" ) );
        ctxt.register_action( "PREV_TAB", _( "Previous page" ) );
        ctxt.register_action( "SCROLL_UP" );
        ctxt.register_action( "SCROLL_DOWN" );
        ctxt.register_action( "CONFIRM" );
        ctxt.register_action( "SELECT_ALL" );
        ctxt.register_action( "QUIT", _( "Cancel" ) );
        ctxt.register_action( "ANY_INPUT" );
        ctxt.register_action( "HELP_KEYBINDINGS" );
        ctxt.register_action( "FILTER" );

        int start = 0;
        int cur_it = 0;
        bool update = true;
        mvwprintw( w_pickup, 0, 0, _( "PICK UP" ) );
        int selected = 0;
        int iScrollPos = 0;

        std::string filter;
        std::string new_filter;
        std::vector<int> matches;//Indexes of items that match the filter
        bool filter_changed = true;
        if( g->was_fullscreen ) {
            g->draw_ter();
        }
        // Now print the two lists; those on the ground and about to be added to inv
        // Continue until we hit return or space
        do {
            const std::string pickup_chars =
                ctxt.get_available_single_char_hotkeys( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:;" );
            int idx = -1;
            for( int i = 1; i < pickupH; i++ ) {
                mvwprintw( w_pickup, i, 0,
                           "                                                " );
            }
            if( action == "ANY_INPUT" &&
                raw_input_char >= '0' && raw_input_char <= '9' ) {
                int raw_input_char_value = ( char )raw_input_char - '0';
                itemcount *= 10;
                itemcount += raw_input_char_value;
                if( itemcount < 0 ) {
                    itemcount = 0;
                }
            } else if( action == "SCROLL_UP" ) {
                iScrollPos--;
            } else if( action == "SCROLL_DOWN" ) {
                iScrollPos++;
            } else if( action == "PREV_TAB" ) {
                if( start > 0 ) {
                    start -= maxitems;
                } else {
                    start = ( int )( ( matches.size() - 1 ) / maxitems ) * maxitems;
                }
                selected = start;
                mvwprintw( w_pickup, maxitems + 2, 0, "         " );
            } else if( action == "NEXT_TAB" ) {
                if( start + maxitems < ( int )matches.size() ) {
                    start += maxitems;
                } else {
                    start = 0;
                }
                iScrollPos = 0;
                selected = start;
                mvwprintw( w_pickup, maxitems + 2, pickupH, "            " );
            } else if( action == "UP" ) {
                selected--;
                iScrollPos = 0;
                if( selected < 0 ) {
                    selected = matches.size() - 1;
                    start = ( int )( matches.size() / maxitems ) * maxitems;
                    if( start >= ( int )matches.size() ) {
                        start -= maxitems;
                    }
                } else if( selected < start ) {
                    start -= maxitems;
                }
            } else if( action == "DOWN" ) {
                selected++;
                iScrollPos = 0;
                if( selected >= ( int )matches.size() ) {
                    selected = 0;
                    start = 0;
                } else if( selected >= start + maxitems ) {
                    start += maxitems;
                }
            } else if( selected >= 0 && selected < int( matches.size() ) &&
                       ( ( action == "RIGHT" && !getitem[matches[selected]].pick ) ||
                         ( action == "LEFT" && getitem[matches[selected]].pick ) ) ) {
                idx = selected;
            } else if( action == "FILTER" ) {
                new_filter = filter;
                string_input_popup popup;
                popup
                .title( _( "Set filter" ) )
                .width( 30 )
                .edit( new_filter );
                if( !popup.canceled() ) {
                    filter_changed = true;
                } else {
                    wrefresh( g->w_terrain );
                }
            } else if( action == "ANY_INPUT" && raw_input_char == '`' ) {
                std::string ext = string_input_popup()
                                  .title( _( "Enter 2 letters (case sensitive):" ) )
                                  .width( 3 )
                                  .max_length( 2 )
                                  .query_string();
                if( ext.size() == 2 ) {
                    int p1 = pickup_chars.find( ext.at( 0 ) );
                    int p2 = pickup_chars.find( ext.at( 1 ) );
                    if( p1 != -1 && p2 != -1 ) {
                        idx = pickup_chars.size() + ( p1 * pickup_chars.size() ) + p2;
                    }
                }
            } else if( action == "ANY_INPUT" ) {
                idx = ( raw_input_char <= 127 ) ? pickup_chars.find( raw_input_char ) : -1;
                iScrollPos = 0;
            }

            if( idx >= 0 && idx < ( int )matches.size() ) {
                size_t true_idx = matches[idx];
                if( itemcount != 0 || getitem[true_idx].count == 0 ) {
                    item &temp = stacked_here[true_idx].begin()->_item;
                    int amount_available = temp.count_by_charges() ? temp.charges : stacked_here[true_idx].size();
                    if( itemcount >= amount_available ) {
                        itemcount = 0;
                    }
                    getitem[true_idx].count = itemcount;
                    itemcount = 0;
                }

                // Note: this might not change the value of getitem[idx] at all!
                getitem[true_idx].pick = ( action == "RIGHT" ? true :
                                           ( action == "LEFT" ? false :
                                             !getitem[true_idx].pick ) );
                if( action != "RIGHT" && action != "LEFT" ) {
                    selected = idx;
                    start = ( int )( idx / maxitems ) * maxitems;
                }

                if( !getitem[true_idx].pick ) {
                    getitem[true_idx].count = 0;
                }
                update = true;
            }
            if( filter_changed ) {
                matches.clear();
                while( matches.empty() ) {
                    auto filter_func = item_filter_from_string( new_filter );
                    for( size_t index = 0; index < stacked_here.size(); index++ ) {
                        if( filter_func( stacked_here[index].begin()->_item ) ) {
                            matches.push_back( index );
                        }
                    }
                    if( matches.empty() ) {
                        popup( _( "Your filter returned no results" ) );
                        wrefresh( g->w_terrain );
                        // The filter must have results, or simply be emptied or canceled,
                        // as this screen can't be reached without there being
                        // items available
                        string_input_popup popup;
                        popup
                        .title( _( "Set filter" ) )
                        .width( 30 )
                        .edit( new_filter );
                        if( popup.canceled() ) {
                            new_filter = filter;
                            filter_changed = false;
                        }
                    }
                }
                if( filter_changed ) {
                    filter = new_filter;
                    filter_changed = false;
                    selected = 0;
                    start = 0;
                    iScrollPos = 0;
                }
                wrefresh( g->w_terrain );
            }
            item &selected_item = stacked_here[matches[selected]].begin()->_item;

            werase( w_item_info );
            if( selected >= 0 && selected <= ( int )stacked_here.size() - 1 ) {
                std::vector<iteminfo> vThisItem;
                std::vector<iteminfo> vDummy;
                selected_item.info( true, vThisItem );

                draw_item_info( w_item_info, "", "", vThisItem, vDummy, iScrollPos, true, true );
            }
            draw_custom_border( w_item_info, false );
            mvwprintw( w_item_info, 0, 2, "< " );
            trim_and_print( w_item_info, 0, 4, itemsW - 8, c_white, "%s >",
                            selected_item.display_name().c_str() );
            wrefresh( w_item_info );

            if( action == "SELECT_ALL" ) {
                int count = 0;
                for( auto i : matches ) {
                    if( getitem[i].pick ) {
                        count++;
                    }
                    getitem[i].pick = true;
                }
                if( count == ( int )stacked_here.size() ) {
                    for( size_t i = 0; i < stacked_here.size(); i++ ) {
                        getitem[i].pick = false;
                    }
                }
                update = true;
            }
            for( cur_it = start; cur_it < start + maxitems; cur_it++ ) {
                mvwprintw( w_pickup, 1 + ( cur_it % maxitems ), 0,
                           "                                        " );
                if( cur_it < ( int )matches.size() ) {
                    int true_it = matches[cur_it];
                    item &this_item = stacked_here[ true_it ].begin()->_item;
                    nc_color icolor = this_item.color_in_inventory();
                    if( cur_it == selected ) {
                        icolor = hilite( icolor );
                    }

                    if( cur_it < ( int )pickup_chars.size() ) {
                        mvwputch( w_pickup, 1 + ( cur_it % maxitems ), 0, icolor,
                                  char( pickup_chars[cur_it] ) );
                    } else if( cur_it < ( int )pickup_chars.size() + ( int )pickup_chars.size() *
                               ( int )pickup_chars.size() ) {
                        int p = cur_it - pickup_chars.size();
                        int p1 = p / pickup_chars.size();
                        int p2 = p % pickup_chars.size();
                        mvwprintz( w_pickup, 1 + ( cur_it % maxitems ), 0, icolor, "`%c%c",
                                   char( pickup_chars[p1] ), char( pickup_chars[p2] ) );
                    } else {
                        mvwputch( w_pickup, 1 + ( cur_it % maxitems ), 0, icolor, ' ' );
                    }
                    if( getitem[true_it].pick ) {
                        if( getitem[true_it].count == 0 ) {
                            wprintz( w_pickup, c_light_blue, " + " );
                        } else {
                            wprintz( w_pickup, c_light_blue, " # " );
                        }
                    } else {
                        wprintw( w_pickup, " - " );
                    }
                    std::string item_name;
                    if( stacked_here[true_it].begin()->_item.ammo_type() == "money" ) {
                        //Count charges
                        //TODO: transition to the item_location system used for the inventory
                        unsigned long charges_total = 0;
                        for( const auto item : stacked_here[true_it] ) {
                            charges_total += item._item.charges;
                        }
                        //Picking up none or all the cards in a stack
                        if( !getitem[true_it].pick || getitem[true_it].count == 0 ) {
                            item_name = stacked_here[true_it].begin()->_item.display_money( stacked_here[true_it].size(),
                                        charges_total );
                        } else {
                            unsigned long charges = 0;
                            int c = getitem[true_it].count;
                            for( auto it = stacked_here[true_it].begin(); it != stacked_here[true_it].end() &&
                                 c > 0; ++it, --c ) {
                                charges += it->_item.charges;
                            }
                            item_name = string_format( _( "%s of %s" ),
                                                       stacked_here[true_it].begin()->_item.display_money( getitem[true_it].count, charges ),
                                                       format_money( charges_total ) );
                        }
                    } else {
                        item_name = this_item.display_name( stacked_here[true_it].size() );
                    }
                    if( stacked_here[true_it].size() > 1 ) {
                        item_name = string_format( "%d %s", stacked_here[true_it].size(), item_name.c_str() );
                    }
                    if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
                        item_name = string_format( "%s %s", this_item.symbol().c_str(),
                                                   item_name.c_str() );
                    }
                    trim_and_print( w_pickup, 1 + ( cur_it % maxitems ), 6, pickupW - 4, icolor,
                                    item_name );
                }
            }

            mvwprintw( w_pickup, maxitems + 1, 0, _( "[%s] Unmark" ),
                       ctxt.get_desc( "LEFT", 1 ).c_str() );

            center_print( w_pickup, maxitems + 1, c_light_gray, string_format( _( "[%s] Help" ),
                          ctxt.get_desc( "HELP_KEYBINDINGS", 1 ).c_str() ) );

            right_print( w_pickup, maxitems + 1, 0, c_light_gray, string_format( _( "[%s] Mark" ),
                         ctxt.get_desc( "RIGHT", 1 ).c_str() ) );

            mvwprintw( w_pickup, maxitems + 2, 0, _( "[%s] Prev" ),
                       ctxt.get_desc( "PREV_TAB", 1 ).c_str() );

            center_print( w_pickup, maxitems + 2, c_light_gray, string_format( _( "[%s] All" ),
                          ctxt.get_desc( "SELECT_ALL", 1 ).c_str() ) );

            right_print( w_pickup, maxitems + 2, 0, c_light_gray, string_format( _( "[%s] Next" ),
                         ctxt.get_desc( "NEXT_TAB", 1 ).c_str() ) );

            if( update ) { // Update weight & volume information
                update = false;
                for( int i = 9; i < pickupW; ++i ) {
                    mvwaddch( w_pickup, 0, i, ' ' );
                }
                units::mass weight_picked_up = 0;
                units::volume volume_picked_up = 0;
                for( size_t i = 0; i < getitem.size(); i++ ) {
                    if( getitem[i].pick ) {
                        item temp = stacked_here[i].begin()->_item;
                        if( temp.count_by_charges() && getitem[i].count < temp.charges && getitem[i].count != 0 ) {
                            temp.charges = getitem[i].count;
                        }
                        int num_picked = std::min( stacked_here[i].size(),
                                                   getitem[i].count == 0 ? stacked_here[i].size() : getitem[i].count );
                        weight_picked_up += temp.weight() * num_picked;
                        volume_picked_up += temp.volume() * num_picked;
                    }
                }

                auto weight_predict = g->u.weight_carried() + weight_picked_up;
                auto volume_predict = g->u.volume_carried() + volume_picked_up;

                mvwprintz( w_pickup, 0, 9, weight_predict > g->u.weight_capacity() ? c_red : c_white,
                           _( "Wgt %.1f" ), round_up( convert_weight( weight_predict ), 1 ) );

                wprintz( w_pickup, c_white, "/%.1f", round_up( convert_weight( g->u.weight_capacity() ), 1 ) );

                std::string fmted_volume_predict = format_volume( volume_predict );
                mvwprintz( w_pickup, 0, 24, volume_predict > g->u.volume_capacity() ? c_red : c_white,
                           _( "Vol %s" ), fmted_volume_predict.c_str() );

                std::string fmted_volume_capacity = format_volume( g->u.volume_capacity() );
                wprintz( w_pickup, c_white, "/%s", fmted_volume_capacity.c_str() );
            };

            wrefresh( w_pickup );

            action = ctxt.handle_input();
            raw_input_char = ctxt.get_raw_input().get_first_input();

        } while( action != "QUIT" && action != "CONFIRM" );

        bool item_selected = false;
        // Check if we have selected an item.
        for( auto selection : getitem ) {
            if( selection.pick ) {
                item_selected = true;
            }
        }
        if( action != "CONFIRM" || !item_selected ) {
            w_pickup = catacurses::window();
            w_item_info = catacurses::window();
            add_msg( _( "Never mind." ) );
            g->reenter_fullscreen();
            g->refresh_all();
            return;
        }
    }

    // At this point we've selected our items, register an activity to pick them up.
    g->u.assign_activity( activity_id( "ACT_PICKUP" ) );
    g->u.activity.placement = pos - g->u.pos();
    g->u.activity.values.push_back( from_vehicle );
    if( min == -1 ) {
        // Auto pickup will need to auto resume since there can be several of them on the stack.
        g->u.activity.auto_resume = true;
    }
    std::vector<std::pair<int, int>> pick_values;
    for( size_t i = 0; i < stacked_here.size(); i++ ) {
        const auto &selection = getitem[i];
        if( !selection.pick ) {
            continue;
        }

        const auto &stack = stacked_here[i];
        // Note: items can be both charged and stacked
        // For robustness, let's assume they can be both in the same stack
        bool pick_all = selection.count == 0;
        size_t count = selection.count;
        for( const item_idx &it : stack ) {
            if( !pick_all && count == 0 ) {
                break;
            }

            if( it._item.count_by_charges() ) {
                size_t num_picked = std::min( ( size_t )it._item.charges, count );
                pick_values.push_back( { static_cast<int>( it.idx ), static_cast<int>( num_picked ) } );
                count -= num_picked;
            } else {
                size_t num_picked = 1;
                pick_values.push_back( { static_cast<int>( it.idx ), 0 } );
                count -= num_picked;
            }
        }
    }
    // The pickup activity picks up items last-to-first from its values list, so make sure the
    // higher indices are at the end.
    std::sort( pick_values.begin(), pick_values.end() );
    for( auto &it : pick_values ) {
        g->u.activity.values.push_back( it.first );
        g->u.activity.values.push_back( it.second );
    }

    g->reenter_fullscreen();
}

//helper function for Pickup::pick_up (singular item)
void remove_from_map_or_vehicle( const tripoint &pos, vehicle *veh, int cargo_part,
                                 int moves_taken, int curmit )
{
    if( veh != nullptr ) {
        veh->remove_item( cargo_part, curmit );
    } else {
        g->m.i_rem( pos, curmit );
    }
    g->u.moves -= moves_taken;
}

//helper function for Pickup::pick_up
void show_pickup_message( const PickupMap &mapPickup )
{
    for( auto &entry : mapPickup ) {
        if( entry.second.first.invlet != 0 ) {
            add_msg( _( "You pick up: %d %s [%c]" ), entry.second.second,
                     entry.second.first.display_name( entry.second.second ).c_str(), entry.second.first.invlet );
        } else {
            add_msg( _( "You pick up: %d %s" ), entry.second.second,
                     entry.second.first.display_name( entry.second.second ).c_str() );
        }
    }
}

int Pickup::cost_to_move_item( const Character &who, const item &it )
{
    // Do not involve inventory capacity, it's not like you put it in backpack
    int ret = 50;
    if( who.is_armed() ) {
        // No free hand? That will cost you extra
        ret += 20;
    }

    // Is it too heavy? It'll take 10 moves per kg over limit
    ret += std::max( 0, ( it.weight() - who.weight_capacity() ) / 100_gram );

    // Keep it sane - it's not a long activity
    return std::min( 400, ret );
}
