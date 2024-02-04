#include "pickup.h"

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "auto_pickup.h"
#include "character.h"
#include "colony.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "input.h"
#include "input_context.h"
#include "item.h"
#include "item_location.h"
#include "item_stack.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "options.h"
#include "player_activity.h"
#include "point.h"
#include "popup.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "units.h"
#include "units_utility.h"

using ItemCount = std::pair<item, int>;
using PickupMap = std::map<std::string, ItemCount>;

static const flag_id json_flag_SHREDDED( "SHREDDED" );
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

enum pickup_answer : int {
    CANCEL = -1,
    WIELD,
    SPILL,
    STASH,
    NUM_ANSWERS
};

static pickup_answer handle_problematic_pickup( const item &it, const std::string &explain )
{
    Character &u = get_player_character();

    uilist amenu;

    amenu.text = explain;

    item empty_it( it );
    empty_it.get_contents().clear_items();
    bool can_pickup = u.can_stash( empty_it ) && u.can_pickWeight( empty_it );

    if( it.is_bucket_nonempty() ) {
        amenu.addentry( WIELD, true, 'w', _( "Wield %s" ), it.display_name() );
        amenu.addentry( SPILL, can_pickup, 's', _( "Spill contents of %s, then pick up %s" ),
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

static bool is_bulk_load( const Pickup::pick_info &lhs, const Pickup::pick_info &rhs )
{
    // Check if storage is the same
    if( lhs.dst && rhs.dst && lhs.dst->stacks_with( *rhs.dst ) &&
        lhs.dst.where() == item_location::type::container &&
        rhs.dst.where() == item_location::type::container &&
        lhs.dst.parent_item() == rhs.dst.parent_item() ) {
        // Check if source is the same
        if( lhs.src_type == rhs.src_type ) {
            switch( lhs.src_type ) {
                case item_location::type::container:
                    return lhs.src_container == rhs.src_container;
                    break;
                case item_location::type::map:
                case item_location::type::vehicle:
                    return lhs.src_pos == rhs.src_pos;
                    break;
                default:
                    break;
            }
        }
    }
    return false;
}

// Returns false if pickup caused a prompt and the player selected to cancel pickup
static bool pick_one_up( item_location &loc, int quantity, bool &got_water, bool &got_gas,
                         PickupMap &mapPickup, bool autopickup, bool &stash_successful, bool &got_frozen_liquid,
                         Pickup::pick_info &info )
{
    Character &player_character = get_player_character();
    bool picked_up = false;
    bool crushed = false;
    Pickup::pick_info pre_info( info );

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
        if( newit.has_flag( json_flag_SHREDDED ) ) {
            option = STASH;
        } else {
            crushed = player_character.crush_frozen_liquid( newloc );
            if( crushed ) {
                option = STASH;
                newit.set_flag( json_flag_SHREDDED );
            } else {
                got_frozen_liquid = true;
            }
        }
    } else if( newit.made_of_from_type( phase_id::LIQUID ) ) {
        got_water = true;
    } else if( newit.made_of_from_type( phase_id::GAS ) ) {
        got_gas = true;
    } else if( newit.is_bucket_nonempty() ) {
        if( !autopickup ) {
            const std::string &explain = string_format( _( "Can't stash %s while it's not empty" ),
                                         newit.display_name() );
            option = handle_problematic_pickup( newit, explain );
            did_prompt = true;
        } else {
            option = CANCEL;
        }
    } else if( !player_character.can_pickWeight_partial( newit, false ) ||
               !player_character.can_stash_partial( newit, false ) ) {
        option = CANCEL;
        stash_successful = false;
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
        case WIELD:
            picked_up = player_character.wield( newit );
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
                const char invlet = newit.invlet;
                newit = it;
                newit.invlet = invlet;
            }
        // Intentional fallthrough
        case STASH: {
            int last_charges = newit.charges;
            ret_val<item_location> ret = player_character.i_add_or_fill( newit, true, nullptr, &it,
                                         /*allow_drop=*/false, /*allow_wield=*/false, false );
            item_location added_it = ret.value();
            if( ret.success() ) {
                if( added_it == item_location::nowhere ) {
                    newit.charges = last_charges - newit.charges;
                    newit.on_pickup( player_character );
                    if( newit.charges != 0 ) {
                        auto &entry = mapPickup[newit.tname()];
                        entry.second += newit.charges;
                        entry.first = newit;
                        picked_up = true;
                    }
                } else if( &*added_it == &it ) {
                    // merged to the original stack, restore original charges
                    it.charges = last_charges;
                } else {
                    // successfully added
                    auto &entry = mapPickup[newit.tname()];
                    entry.second += newit.count();
                    entry.first = newit;
                    picked_up = true;
                }
            }
            if( picked_up ) {
                // Update info
                info.set_dst( added_it );
            }
            break;
        }
    }

    if( picked_up ) {
        info.set_src( loc );
        info.total_bulk_volume += loc->volume( false, false, quantity );
        if( !is_bulk_load( pre_info, info ) ) {
            // Cost to take an item from a container or map
            player_character.moves -= loc.obtain_cost( player_character, quantity );
        } else {
            // Pure cost to handling item excluding overhead.
            player_character.moves -= std::max( player_character.item_handling_cost( *loc, true, 0, quantity,
                                                true ), 1 );
        }
        contents_change_handler handler;
        handler.unseal_pocket_containing( loc );
        item &orig_it = *loc.get_item();
        // Subtract moved charges instead of assigning leftover charges,
        // since the total charges of the original item may have changed
        // due to merging.
        if( orig_it.charges > newit.charges ) {
            orig_it.charges -= newit.charges;
        } else {
            loc.remove_item();
        }
        player_character.flag_encumbrance();
        player_character.invalidate_weight_carried_cache();
    }

    return picked_up || !did_prompt;
}

bool Pickup::do_pickup( std::vector<item_location> &targets, std::vector<int> &quantities,
                        bool autopickup,
                        bool &stash_successful, Pickup::pick_info &info )
{
    bool got_water = false;
    bool got_gas = false;
    bool got_frozen_liquid = false;
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
        problem = !pick_one_up( target, quantity, got_water, got_gas, mapPickup, autopickup,
                                stash_successful, got_frozen_liquid, info );
        if( info.total_bulk_volume > 200_ml ) {
            // Bulk loading is not allowed beyond a certain volume
            info = Pickup::pick_info();
        }
    }

    if( !mapPickup.empty() ) {
        show_pickup_message( mapPickup );
    }
    if( got_frozen_liquid ) {
        add_msg( m_info, _( "Chunks of frozen liquid cannot be picked up without the correct tools." ) );
    }
    if( got_water ) {
        add_msg( m_info, _( "Spilt liquid cannot be picked back up.  Try mopping it instead." ) );
    }
    if( got_gas ) {
        add_msg( m_info, _( "Spilt gasses cannot be picked up.  They will disappear over time." ) );
    }
    if( weight_is_okay && player_character.weight_carried() > player_character.weight_capacity() ) {
        add_msg( m_bad, _( "You're overburdened!" ) );
    }

    player_character.recoil = MAX_RECOIL;

    return !problem;
}

// Auto pickup items at given location
void Pickup::autopickup( const tripoint &p )
{
    map &local = get_map();

    if( local.i_at( p ).empty() && !get_option<bool>( "AUTO_PICKUP_ADJACENT" ) ) {
        return;
    }
    // which items are we grabbing?
    std::vector<item_stack::iterator> here;
    const map_stack mapitems = local.i_at( p );
    for( item_stack::iterator it = mapitems.begin(); it != mapitems.end(); ++it ) {
        here.push_back( it );
    }
    Character &player = get_player_character();
    // Recursively pick up adjacent items if that option is on.
    if( get_option<bool>( "AUTO_PICKUP_ADJACENT" ) && player.pos() == p ) {
        //Autopickup adjacent
        std::array<direction, 8> adjacentDir = {
            direction::NORTH, direction::NORTHEAST, direction::EAST,
            direction::SOUTHEAST, direction::SOUTH, direction::SOUTHWEST,
            direction::WEST, direction::NORTHWEST
        };
        for( direction &elem : adjacentDir ) {

            tripoint apos = tripoint( displace_XY( elem ), 0 );
            apos += p;

            autopickup( apos );
        }
    }
    // Bail out if this square cannot be auto-picked-up
    if( g->check_zone( zone_type_NO_AUTO_PICKUP, p ) ||
        local.has_flag( ter_furn_flag::TFLAG_SEALED, p ) ) {
        return;
    }
    drop_locations selected_items = auto_pickup::select_items( here, p );
    if( selected_items.empty() ) {
        return;
    }
    // At this point we've selected our items, register an activity to pick them up.
    std::vector<int> quantities;
    std::vector<item_location> target_items;
    for( drop_location selected : selected_items ) {
        item *it = selected.first.get_item();
        target_items.push_back( selected.first );
        quantities.push_back( it->count_by_charges() ? it->charges : 0 );
    }
    pickup_activity_actor actor( target_items, quantities, player.pos(), true );
    player.assign_activity( actor );

    // Auto pickup will need to auto resume since there can be several of them on the stack.
    player.activity.auto_resume = true;
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

void Pickup::pick_info::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "total_bulk_volume", total_bulk_volume );
    jsout.member( "src_type", static_cast<int>( src_type ) );
    jsout.member( "src_pos", src_pos );
    jsout.member( "src_container", src_container );
    jsout.member( "dst", dst );
    jsout.end_object();
}

void Pickup::pick_info::deserialize( const JsonObject &jsobj )
{
    int src_type_ = 0;
    jsobj.read( "total_bulk_volume", total_bulk_volume );
    jsobj.read( "src_type", src_type_ );
    src_type = static_cast<item_location::type>( src_type_ );
    jsobj.read( "src_pos", src_pos );
    jsobj.read( "src_container", src_container );
    jsobj.read( "dst", dst );
}

void Pickup::pick_info::set_src( const item_location &src_ )
{
    // item_location of source may become invalid after the item is moved, so save the information separately.
    src_pos = src_.position();
    src_container = src_.parent_item();
    src_type = src_.where();
}

void Pickup::pick_info::set_dst( const item_location &dst_ )
{
    dst = dst_;
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
