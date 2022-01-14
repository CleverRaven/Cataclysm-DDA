#include "pickup.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "auto_pickup.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "colony.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "input.h"
#include "item.h"
#include "item_location.h"
#include "item_search.h"
#include "item_stack.h"
#include "line.h"
#include "map.h"
#include "map_selector.h"
#include "mapdata.h"
#include "messages.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "panels.h"
#include "player_activity.h"
#include "point.h"
#include "popup.h"
#include "ret_val.h"
#include "sdltiles.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "units.h"
#include "units_utility.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"

using ItemCount = std::pair<item, int>;
using PickupMap = std::map<std::string, ItemCount>;

static const zone_type_id zone_type_NO_AUTO_PICKUP( "NO_AUTO_PICKUP" );

//helper function for Pickup::autopickup
static void show_pickup_message( const PickupMap &mapPickup )
{
    for( const auto &entry : mapPickup ) {
        if( entry.second.first.invlet != 0 ) {
            add_msg( _( "You pick up: %d %s [%c]" ), entry.second.second,
                     entry.second.first.display_name( entry.second.second ), entry.second.first.invlet );
        } else if( entry.second.first.count_by_charges() ) {
            add_msg( _( "You pick up: %s" ), entry.second.first.display_name( entry.second.second ) );
        } else {
            add_msg( _( "You pick up: %d %s" ), entry.second.second,
                     entry.second.first.display_name( entry.second.second ) );
        }
    }
}

struct pickup_count {
    bool pick = false;
    //count is 0 if the whole stack is being picked up, nonzero otherwise.
    int count = 0;
};

static bool is_valid_auto_pickup( const item *pickup_item )
{
    int weight_limit = get_option<int>( "AUTO_PICKUP_WEIGHT_LIMIT" );
    int volume_limit = get_option<int>( "AUTO_PICKUP_VOL_LIMIT" );

    return pickup_item->volume() > units::from_milliliter( volume_limit * 50 ) ||
           pickup_item->weight() > weight_limit * 50_gram;
}

static bool should_auto_pickup( const item *pickup_item )
{
    std::string item_name = pickup_item->tname( 1, false );
    rule_state pickup_state = get_auto_pickup().check_item( item_name );

    if( !is_valid_auto_pickup( pickup_item ) ) {
        return false;
    } else if( pickup_state == rule_state::WHITELISTED ) {
        return true;
    } else if( pickup_state != rule_state::BLACKLISTED ) {
        //No prematched pickup rule found, check rules in more detail
        get_auto_pickup().create_rule( pickup_item );

        if( get_auto_pickup().check_item( item_name ) == rule_state::WHITELISTED ) {
            return true;
        }
    }
    return false;
}

static std::vector<item_location *> get_pickup_list_from( item_location *container )
{
    std::vector<item_location *> pickup_list;
    std::list<item *> contents = container->get_item()->get_contents().all_items_top();
    pickup_list.reserve( contents.size() );

    for( item *item_to_check : contents ) {
        if( should_auto_pickup( item_to_check ) ) {
            // TODO: make check here for BLACKLIST
            pickup_list.push_back( new item_location( *container, item_to_check ) );
        } else if( !item_to_check->is_container_empty() ) {
            // get pickup list from nested item container
            item_location *location = new item_location( *container, item_to_check );
            std::vector<item_location *> pickup_nested = get_pickup_list_from( location );

            pickup_list.reserve( pickup_nested.size() + pickup_list.size() );
            pickup_list.insert( pickup_list.end(), pickup_nested.begin(), pickup_nested.end() );
        }
    }
    // all items in container were approved for pickup
    if( !contents.empty() && pickup_list.size() == contents.size() ) {
        // picking up whole container so delete all registered pickups
        for( item_location *dealoc : pickup_list ) {
            delete( dealoc );
        }
        pickup_list.clear();
        pickup_list.push_back( container );
    } else {
        // container will not be picked up
        delete( container );
    }
    return pickup_list;
}

static bool select_autopickup_items( std::vector<std::list<item_stack::iterator>> &here,
                                     std::vector<pickup_count> &getitem,
                                     std::vector<item_location> &target_items,
                                     const tripoint &location )
{
    bool bFoundSomething = false;
    const map_cursor map_location = map_cursor( location );

    for( size_t iVol = 0, iNumChecked = 0; iNumChecked < here.size(); iVol++ ) {
        // iterate over all item stacks found in location
        for( size_t i = 0; i < here.size(); i++ ) {
            item_stack::iterator begin_iterator = here[i].front();
            if( begin_iterator->volume() / units::legacy_volume_factor == static_cast<int>( iVol ) ) {
                iNumChecked++;
                item *item_entry = &*begin_iterator;
                const std::string sItemName = item_entry->tname( 1, false );
                item_contents &contents = begin_iterator->get_contents();

                // before checking contents check if item is on pickup list
                if( should_auto_pickup( &*begin_iterator ) ) {
                    getitem[i].pick = true;
                    bFoundSomething = true;
                } else if( begin_iterator->is_container() && !begin_iterator->empty_container() ) {
                    item_location *container_location = new item_location( map_location, item_entry );
                    for( item_location *add_item : get_pickup_list_from( container_location ) ) {
                        target_items.insert( target_items.begin(), *add_item );
                        bFoundSomething = true;
                    }
                }
            }
        }
    }
    return bFoundSomething;
}

enum pickup_answer : int {
    CANCEL = -1,
    SPILL,
    STASH,
    NUM_ANSWERS
};

static pickup_answer handle_problematic_pickup( const item &it, const std::string &explain )
{
    Character &u = get_player_character();

    uilist amenu;

    amenu.text = explain;

    if( it.is_bucket_nonempty() ) {
        amenu.addentry( SPILL, u.can_stash( it ), 's', _( "Spill contents of %s, then pick up %s" ),
                        it.tname(), it.display_name() );
    }

    amenu.query();
    int choice = amenu.ret;

    if( choice <= CANCEL || choice >= NUM_ANSWERS ) {
        return CANCEL;
    }

    return static_cast<pickup_answer>( choice );
}

bool Pickup::query_thief()
{
    Character &u = get_player_character();
    const bool force_uc = get_option<bool>( "FORCE_CAPITAL_YN" );
    const auto &allow_key = force_uc ? input_context::disallow_lower_case_or_non_modified_letters
                            : input_context::allow_all_keys;
    std::string answer = query_popup()
                         .preferred_keyboard_mode( keyboard_mode::keycode )
                         .allow_cancel( false )
                         .context( "YES_NO_ALWAYS_NEVER" )
                         .message( "%s", force_uc && !is_keycode_mode_supported()
                                   ? _( "Picking up this item will be considered stealing, continue?  (Case sensitive)" )
                                   : _( "Picking up this item will be considered stealing, continue?" ) )
                         .option( "YES", allow_key ) // yes, steal all items in this location that is selected
                         .option( "NO", allow_key ) // no, pick up only what is free
                         .option( "ALWAYS", allow_key ) // Yes, steal all items and stop asking me this question
                         .option( "NEVER", allow_key ) // no, only grab free item and never ask me again
                         .cursor( 1 ) // default to the second option `NO`
                         .query()
                         .action; // retrieve the input action
    if( answer == "YES" ) {
        u.set_value( "THIEF_MODE", "THIEF_STEAL" );
        u.set_value( "THIEF_MODE_KEEP", "NO" );
        return true;
    } else if( answer == "NO" ) {
        u.set_value( "THIEF_MODE", "THIEF_HONEST" );
        u.set_value( "THIEF_MODE_KEEP", "NO" );
        return false;
    } else if( answer == "ALWAYS" ) {
        u.set_value( "THIEF_MODE", "THIEF_STEAL" );
        u.set_value( "THIEF_MODE_KEEP", "YES" );
        return true;
    } else if( answer == "NEVER" ) {
        u.set_value( "THIEF_MODE", "THIEF_HONEST" );
        u.set_value( "THIEF_MODE_KEEP", "YES" );
        return false;
    } else {
        // error
        debugmsg( "Not a valid option [ %s ]", answer );
    }
    return false;
}

// Returns false if pickup caused a prompt and the player selected to cancel pickup
static bool pick_one_up( item_location &loc, int quantity, bool &got_water, PickupMap &mapPickup,
                         bool autopickup, bool &stash_successful )
{
    Character &player_character = get_player_character();
    int moves_taken = loc.obtain_cost( player_character, quantity );
    bool picked_up = false;
    pickup_answer option = CANCEL;

    // We already checked in do_pickup if this was a nullptr
    // Make copies so the original remains untouched if we bail out
    item_location newloc = loc;
    //original item reference
    item &it = *newloc.get_item();
    //new item (copy)
    item newit = it;

    if( !newit.is_owned_by( player_character, true ) ) {
        // Has the player given input on if stealing is ok?
        if( player_character.get_value( "THIEF_MODE" ) == "THIEF_ASK" ) {
            Pickup::query_thief();
        }
        if( player_character.get_value( "THIEF_MODE" ) == "THIEF_HONEST" ) {
            return true; // Since we are honest, return no problem before picking up
        }
    }
    if( newit.invlet != '\0' &&
        player_character.invlet_to_item( newit.invlet ) != nullptr ) {
        // Existing invlet is not re-usable, remove it and let the code in player.cpp/inventory.cpp
        // add a new invlet, otherwise keep the (usable) invlet.
        newit.invlet = '\0';
    }

    // Handle charges, quantity == 0 means move all
    if( quantity != 0 && newit.count_by_charges() ) {
        if( newit.charges > quantity ) {
            newit.charges = quantity;
        }
    }

    bool did_prompt = false;
    if( newit.is_frozen_liquid() ) {
        if( !( got_water = !player_character.crush_frozen_liquid( newloc ) ) ) {
            option = STASH;
        }
    } else if( newit.made_of_from_type( phase_id::LIQUID ) && !newit.is_frozen_liquid() ) {
        got_water = true;
    } else if( !player_character.can_pickWeight_partial( newit, false ) ||
               !player_character.can_stash_partial( newit ) ) {
        option = CANCEL;
        stash_successful = false;
    } else if( newit.is_bucket_nonempty() ) {
        if( !autopickup ) {
            const std::string &explain = string_format( _( "Can't stash %s while it's not empty" ),
                                         newit.display_name() );
            option = handle_problematic_pickup( newit, explain );
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
        case SPILL:
            if( newit.is_container_empty() ) {
                debugmsg( "Tried to spill contents from an empty container" );
                break;
            }
            //using original item, possibly modifying it
            picked_up = it.spill_contents( player_character );
            if( !picked_up ) {
                break;
            } else {
                const int invlet = newit.invlet;
                newit = it;
                newit.invlet = invlet;
            }
        // Intentional fallthrough
        case STASH: {
            item &added_it = player_character.i_add( newit, true, nullptr,
                             &it, /*allow_drop=*/false, /*allow_wield=*/false );
            if( added_it.is_null() ) {
                // failed to add, fill pockets if it's a stack
                if( newit.count_by_charges() ) {
                    int remaining_charges = newit.charges;
                    item &weapon = player_character.get_wielded_item();
                    if( weapon.can_contain_partial( newit ) ) {
                        int used_charges = weapon.fill_with( newit, remaining_charges );
                        remaining_charges -= used_charges;
                    }
                    for( item &i : player_character.worn ) {
                        if( remaining_charges == 0 ) {
                            break;
                        }
                        if( i.can_contain_partial( newit ) ) {
                            int used_charges = i.fill_with( newit, remaining_charges );
                            remaining_charges -= used_charges;
                        }
                    }
                    newit.charges -= remaining_charges;
                    newit.on_pickup( player_character );
                    if( newit.charges != 0 ) {
                        auto &entry = mapPickup[newit.tname()];
                        entry.second += newit.charges;
                        entry.first = newit;
                        picked_up = true;
                    }
                }
            } else if( &added_it == &it ) {
                // merged to the original stack, restore original charges
                it.charges -= newit.charges;
            } else {
                // successfully added
                auto &entry = mapPickup[newit.tname()];
                entry.second += newit.count();
                entry.first = newit;
                picked_up = true;
            }

            break;
        }
    }

    if( picked_up ) {
        item &orig_it = *loc.get_item();
        // Subtract moved charges instead of assigning leftover charges,
        // since the total charges of the original item may have changed
        // due to merging.
        if( orig_it.charges > newit.charges ) {
            orig_it.charges -= newit.charges;
        } else {
            loc.remove_item();
        }
        player_character.moves -= moves_taken;
        player_character.flag_encumbrance();
        player_character.invalidate_weight_carried_cache();
    }

    return picked_up || !did_prompt;
}

bool Pickup::do_pickup( std::vector<item_location> &targets, std::vector<int> &quantities,
                        bool autopickup, bool &stash_successful )
{
    bool got_water = false;
    Character &player_character = get_player_character();
    bool weight_is_okay = ( player_character.weight_carried() <= player_character.weight_capacity() );

    // Map of items picked up so we can output them all at the end and
    // merge dropping items with the same name.
    PickupMap mapPickup;

    bool problem = false;
    while( !problem && player_character.moves >= 0 && !targets.empty() ) {
        item_location target = std::move( targets.back() );
        int quantity = quantities.back();
        // Whether we pick the item up or not, we're done trying to do so,
        // so remove it from the list.
        targets.pop_back();
        quantities.pop_back();

        if( !target ) {
            debugmsg( "lost target item of ACT_PICKUP" );
            continue;
        }

        problem = !pick_one_up( target, quantity, got_water, mapPickup, autopickup, stash_successful );
    }

    if( !mapPickup.empty() ) {
        show_pickup_message( mapPickup );
    }

    if( got_water ) {
        add_msg( m_info, _( "Spilt liquid cannot be picked back up.  Try mopping it instead." ) );
    }
    if( weight_is_okay && player_character.weight_carried() > player_character.weight_capacity() ) {
        add_msg( m_bad, _( "You're overburdened!" ) );
    }

    return !problem;
}

// Pick up items at (pos).
void Pickup::autopickup( const tripoint &p )
{
    map &local = get_map();

    if( local.i_at( p ).empty() && !get_option<bool>( "AUTO_PICKUP_ADJACENT" ) ) {
        return;
    }

    // which items are we grabbing?
    std::vector<item_stack::iterator> here;
    map_stack mapitems = local.i_at( p );
    for( item_stack::iterator it = mapitems.begin(); it != mapitems.end(); ++it ) {
        here.push_back( it );
    }

    Character &player_character = get_player_character();
    // Recursively pick up adjacent items if that option is on.
    if( get_option<bool>( "AUTO_PICKUP_ADJACENT" ) && player_character.pos() == p ) {
        //Autopickup adjacent
        direction adjacentDir[8] = {direction::NORTH, direction::NORTHEAST, direction::EAST, direction::SOUTHEAST, direction::SOUTH, direction::SOUTHWEST, direction::WEST, direction::NORTHWEST};
        for( auto &elem : adjacentDir ) {

            tripoint apos = tripoint( displace_XY( elem ), 0 );
            apos += p;

            autopickup( apos );
        }
    }

    // Bail out if this square cannot be auto-picked-up
    if( g->check_zone( zone_type_NO_AUTO_PICKUP, p ) ) {
        return;
    }
    if( local.has_flag( ter_furn_flag::TFLAG_SEALED, p ) ) {
        return;
    }

    std::vector<std::list<item_stack::iterator>> stacked_here;
    for( const item_stack::iterator &it : here ) {
        bool found_stack = false;
        for( std::list<item_stack::iterator> &stack : stacked_here ) {
            if( stack.front()->display_stacked_with( *it ) ) {
                stack.push_back( it );
                found_stack = true;
                break;
            }
        }
        if( !found_stack ) {
            stacked_here.emplace_back( std::list<item_stack::iterator>( { it } ) );
        }
    }

    // Items are stored unordered in colonies on the map, so sort them for a nice display.
    std::sort( stacked_here.begin(), stacked_here.end(), []( const auto & lhs, const auto & rhs ) {
        return *lhs.front() < *rhs.front();
    } );

    std::vector<pickup_count> getitem( stacked_here.size() );
    std::vector<item_location> target_items;

    if( !select_autopickup_items( stacked_here, getitem, target_items, p ) ) {
        // If we didn't find anything, bail out now.
        return;
    }
    // At this point we've selected our items, register an activity to pick them up.
    std::vector<std::pair<item_stack::iterator, int>> pick_values;
    for( size_t i = 0; i < stacked_here.size(); i++ ) {
        const pickup_count &selection = getitem[i];
        if( !selection.pick ) {
            continue;
        }

        const std::list<item_stack::iterator> &stack = stacked_here[i];
        // Note: items can be both charged and stacked
        // For robustness, let's assume they can be both in the same stack
        bool pick_all = selection.count == 0;
        int count = selection.count;
        for( const item_stack::iterator &it : stack ) {
            if( !pick_all && count == 0 ) {
                break;
            }

            if( it->count_by_charges() ) {
                int num_picked = std::min( it->charges, count );
                pick_values.emplace_back( it, num_picked );
                count -= num_picked;
            } else {
                pick_values.emplace_back( it, 0 );
                --count;
            }
        }
    }
    std::vector<int> quantities;
    // create quantities for already registered target items to pickup
    for( size_t i = 0; i < target_items.size(); i++ ) {
        quantities.push_back( 0 );
    }
    for( std::pair<item_stack::iterator, int> &iter_qty : pick_values ) {
        target_items.emplace_back( map_cursor( p ), &*iter_qty.first );
        quantities.push_back( iter_qty.second );
    }
    player_character.assign_activity( player_activity( pickup_activity_actor( target_items, quantities,
                                      player_character.pos() ) ) );
    // Auto pickup will need to auto resume since there can be several of them on the stack.
    player_character.activity.auto_resume = true;
}

int Pickup::cost_to_move_item( const Character &who, const item &it )
{
    // Do not involve inventory capacity, it's not like you put it in backpack
    int ret = 50;
    if( who.is_armed() ) {
        // No free hand? That will cost you extra
        ret += 20;
    }
    const int delta_weight = units::to_gram( it.weight() - who.weight_capacity() );
    // Is it too heavy? It'll take 10 moves per kg over limit
    ret += std::max( 0, delta_weight / 100 );

    // Keep it sane - it's not a long activity
    return std::min( 400, ret );
}

std::vector<Pickup::pickup_rect> Pickup::pickup_rect::list;

Pickup::pickup_rect *Pickup::pickup_rect::find_by_coordinate( const point &p )
{
    for( pickup_rect &rect : pickup_rect::list ) {
        if( rect.contains( p ) ) {
            return &rect;
        }
    }
    return nullptr;
}
