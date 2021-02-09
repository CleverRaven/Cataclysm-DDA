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

static const itype_id itype_water( "water" );

// Pickup helper functions
static bool pick_one_up( item_location &loc, int quantity, bool &got_water, bool &offered_swap,
                         PickupMap &mapPickup, bool autopickup );

static void show_pickup_message( const PickupMap &mapPickup );

struct pickup_count {
    bool pick = false;
    //count is 0 if the whole stack is being picked up, nonzero otherwise.
    int count = 0;
};

static bool select_autopickup_items( std::vector<std::list<item_stack::iterator>> &here,
                                     std::vector<pickup_count> &getitem )
{
    bool bFoundSomething = false;

    //Loop through Items lowest Volume first
    bool bPickup = false;

    for( size_t iVol = 0, iNumChecked = 0; iNumChecked < here.size(); iVol++ ) {
        for( size_t i = 0; i < here.size(); i++ ) {
            bPickup = false;
            item_stack::iterator begin_iterator = here[i].front();
            if( begin_iterator->volume() / units::legacy_volume_factor == static_cast<int>( iVol ) ) {
                iNumChecked++;
                const std::string sItemName = begin_iterator->tname( 1, false );

                //Check the Pickup Rules
                if( get_auto_pickup().check_item( sItemName ) == rule_state::WHITELISTED ) {
                    bPickup = true;
                } else if( get_auto_pickup().check_item( sItemName ) != rule_state::BLACKLISTED ) {
                    //No prematched pickup rule found
                    //check rules in more detail
                    get_auto_pickup().create_rule( &*begin_iterator );

                    if( get_auto_pickup().check_item( sItemName ) == rule_state::WHITELISTED ) {
                        bPickup = true;
                    }
                }

                //Auto Pickup all items with Volume <= AUTO_PICKUP_VOL_LIMIT * 50 and Weight <= AUTO_PICKUP_ZERO * 50
                //items will either be in the autopickup list ("true") or unmatched ("")
                if( !bPickup ) {
                    int weight_limit = get_option<int>( "AUTO_PICKUP_WEIGHT_LIMIT" );
                    int volume_limit = get_option<int>( "AUTO_PICKUP_VOL_LIMIT" );
                    if( weight_limit && volume_limit ) {
                        if( begin_iterator->volume() <= units::from_milliliter( volume_limit * 50 ) &&
                            begin_iterator->weight() <= weight_limit * 50_gram &&
                            get_auto_pickup().check_item( sItemName ) != rule_state::BLACKLISTED ) {
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

static pickup_answer handle_problematic_pickup( const item &it, bool &offered_swap,
        const std::string &explain )
{
    if( offered_swap ) {
        return CANCEL;
    }

    Character &u = get_player_character();

    uilist amenu;

    amenu.text = explain;

    offered_swap = true;
    // TODO: Gray out if not enough hands
    if( u.has_wield_conflicts( it ) ) {
        amenu.addentry( WIELD, u.can_unwield( u.weapon ).success(), 'w',
                        _( "Dispose of %s and wield %s" ), u.weapon.display_name(),
                        it.display_name() );
    } else {
        amenu.addentry( WIELD, true, 'w', _( "Wield %s" ), it.display_name() );
    }
    if( it.is_armor() ) {
        amenu.addentry( WEAR, u.can_wear( it ).success(), 'W', _( "Wear %s" ), it.display_name() );
    }
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
bool pick_one_up( item_location &loc, int quantity, bool &got_water, bool &offered_swap,
                  PickupMap &mapPickup, bool autopickup )
{
    Character &player_character = get_player_character();
    int moves_taken = 100;
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
        if( !( got_water = !( player_character.crush_frozen_liquid( newloc ) ) ) ) {
            option = STASH;
        }
    } else if( newit.made_of_from_type( phase_id::LIQUID ) && !newit.is_frozen_liquid() ) {
        got_water = true;
    } else if( !player_character.can_pickWeight_partial( newit, false ) ) {
        if( !autopickup ) {
            const std::string &explain = string_format( _( "The %s is too heavy!" ),
                                         newit.display_name() );
            option = handle_problematic_pickup( newit, offered_swap, explain );
            did_prompt = true;
        } else {
            option = CANCEL;
        }
    } else if( newit.is_bucket_nonempty() ) {
        if( !autopickup ) {
            const std::string &explain = string_format( _( "Can't stash %s while it's not empty" ),
                                         newit.display_name() );
            option = handle_problematic_pickup( newit, offered_swap, explain );
            did_prompt = true;
        } else {
            option = CANCEL;
        }
    } else if( !player_character.can_stash_partial( newit ) ) {
        if( !autopickup ) {
            const std::string &explain = string_format( _( "Not enough capacity to stash %s" ),
                                         newit.display_name() );
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
            picked_up = !!player_character.wear_item( newit );
            break;
        case WIELD: {
            const auto wield_check = player_character.can_wield( newit );
            if( wield_check.success() ) {
                picked_up = player_character.wield( newit );
                if( player_character.weapon.invlet ) {
                    add_msg( m_info, _( "Wielding %c - %s" ), player_character.weapon.invlet,
                             player_character.weapon.display_name() );
                } else {
                    add_msg( m_info, _( "Wielding - %s" ), player_character.weapon.display_name() );
                }
            } else {
                add_msg( m_neutral, wield_check.c_str() );
            }
            break;
        }
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
            item &added_it = player_character.i_add( newit, true, nullptr, /*allow_drop=*/false,
                             !newit.count_by_charges() );
            if( added_it.is_null() ) {
                // failed to add, fill pockets if it's a stack
                if( newit.count_by_charges() ) {
                    int remaining_charges = newit.charges;
                    if( player_character.weapon.can_contain_partial( newit ) ) {
                        int used_charges = player_character.weapon.fill_with( newit, remaining_charges );
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
                        bool autopickup )
{
    bool got_water = false;
    Character &player_character = get_player_character();
    bool weight_is_okay = ( player_character.weight_carried() <= player_character.weight_capacity() );
    bool offered_swap = false;

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

        problem = !pick_one_up( target, quantity, got_water, offered_swap, mapPickup, autopickup );
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
void Pickup::pick_up( const tripoint &p, int min, from_where get_items_from )
{
    int cargo_part = -1;

    map &local = get_map();
    const optional_vpart_position vp = local.veh_at( p );
    vehicle *const veh = veh_pointer_or_null( vp );
    bool from_vehicle = false;

    if( min != -1 ) {
        if( veh != nullptr && get_items_from == prompt ) {
            const cata::optional<vpart_reference> carg = vp.part_with_feature( "CARGO", false );
            const bool veh_has_items = carg && !veh->get_items( carg->part_index() ).empty();
            const bool map_has_items = local.has_items( p );
            if( veh_has_items && map_has_items ) {
                uilist amenu( _( "Get items from where?" ), { _( "Get items from vehicle cargo" ), _( "Get items on the ground" ) } );
                if( amenu.ret == UILIST_CANCEL ) {
                    return;
                }
                get_items_from = static_cast<from_where>( amenu.ret );
            } else if( veh_has_items ) {
                get_items_from = from_cargo;
            }
        }
        if( get_items_from == from_cargo ) {
            const cata::optional<vpart_reference> carg = vp.part_with_feature( "CARGO", false );
            cargo_part = carg ? carg->part_index() : -1;
            from_vehicle = cargo_part >= 0;
        } else {
            // Nothing to change, default is to pick from ground anyway.
            if( local.has_flag( "SEALED", p ) ) {
                return;
            }
        }
    }

    if( !from_vehicle ) {
        bool isEmpty = ( local.i_at( p ).empty() );

        // Hide the pickup window if this is a toilet and there's nothing here
        // but non-frozen water.
        if( ( !isEmpty ) && local.furn( p ) == f_toilet ) {
            isEmpty = true;
            for( const item &maybe_water : local.i_at( p ) ) {
                if( maybe_water.typeId() != itype_water  || maybe_water.is_frozen_liquid() ) {
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
    std::vector<item_stack::iterator> here;
    if( from_vehicle ) {
        vehicle_stack vehitems = veh->get_items( cargo_part );
        for( item_stack::iterator it = vehitems.begin(); it != vehitems.end(); ++it ) {
            here.push_back( it );
        }
    } else {
        map_stack mapitems = local.i_at( p );
        for( item_stack::iterator it = mapitems.begin(); it != mapitems.end(); ++it ) {
            here.push_back( it );
        }
    }

    Character &player_character = get_player_character();
    if( min == -1 ) {
        // Recursively pick up adjacent items if that option is on.
        if( get_option<bool>( "AUTO_PICKUP_ADJACENT" ) && player_character.pos() == p ) {
            //Autopickup adjacent
            direction adjacentDir[8] = {direction::NORTH, direction::NORTHEAST, direction::EAST, direction::SOUTHEAST, direction::SOUTH, direction::SOUTHWEST, direction::WEST, direction::NORTHWEST};
            for( auto &elem : adjacentDir ) {

                tripoint apos = tripoint( direction_XY( elem ), 0 );
                apos += p;

                pick_up( apos, min );
            }
        }

        // Bail out if this square cannot be auto-picked-up
        if( g->check_zone( zone_type_id( "NO_AUTO_PICKUP" ), p ) ) {
            return;
        } else if( local.has_flag( "SEALED", p ) ) {
            return;
        }
    }

    // Not many items, just grab them
    if( static_cast<int>( here.size() ) <= min && min != -1 ) {
        if( from_vehicle ) {
            player_character.assign_activity( player_activity( pickup_activity_actor(
            { item_location( vehicle_cursor( *veh, cargo_part ), &*here.front() ) },
            { 0 },
            cata::nullopt
                                              ) ) );
        } else {
            player_character.assign_activity( player_activity( pickup_activity_actor(
            {item_location( map_cursor( p ), &*here.front() ) },
            { 0 },
            player_character.pos()
                                              ) ) );
        }
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

    if( min == -1 ) { //Auto Pickup, select matching items
        if( !select_autopickup_items( stacked_here, getitem ) ) {
            // If we didn't find anything, bail out now.
            return;
        }
    } else {
        g->temp_exit_fullscreen();

        int start = 0;
        int selected = 0;
        int maxitems = 0;
        int pickupH = 0;
        int pickupW = 44;
        int pickupX = 0;
        catacurses::window w_pickup;
        catacurses::window w_item_info;

        ui_adaptor ui;
        ui.on_screen_resize( [&]( ui_adaptor & ui ) {
            const int itemsH = std::min( 25, TERMY / 2 );
            const int pickupBorderRows = 4;

            // The pickup list may consume the entire terminal, minus space needed for its
            // header/footer and the item info window.
            const int minleftover = itemsH + pickupBorderRows;
            const int maxmaxitems = TERMY - minleftover;
            const int minmaxitems = 9;
            maxitems = clamp<int>( stacked_here.size(), minmaxitems, maxmaxitems );

            start = selected - selected % maxitems;

            pickupH = maxitems + pickupBorderRows;

            //find max length of item name and resize pickup window width
            for( const std::list<item_stack::iterator> &cur_list : stacked_here ) {
                const item &this_item = *cur_list.front();
                const int item_len = utf8_width( remove_color_tags( this_item.display_name() ) ) + 10;
                if( item_len > pickupW && item_len < TERMX ) {
                    pickupW = item_len;
                }
            }

            pickupX = 0;
            std::string position = get_option<std::string>( "PICKUP_POSITION" );
            if( position == "left" ) {
                pickupX = panel_manager::get_manager().get_width_left();
            } else if( position == "right" ) {
                pickupX = TERMX - panel_manager::get_manager().get_width_right() - pickupW;
            } else if( position == "overlapping" ) {
                if( get_option<std::string>( "SIDEBAR_POSITION" ) == "right" ) {
                    pickupX = TERMX - pickupW;
                }
            }

            w_pickup = catacurses::newwin( pickupH, pickupW, point( pickupX, 0 ) );
            w_item_info = catacurses::newwin( TERMY - pickupH, pickupW,
                                              point( pickupX, pickupH ) );

            ui.position( point( pickupX, 0 ), point( pickupW, TERMY ) );
        } );
        ui.mark_resize();

        int itemcount = 0;

        std::string action;
        int raw_input_char = ' ';
        input_context ctxt( "PICKUP", keyboard_mode::keychar );
        ctxt.register_action( "UP" );
        ctxt.register_action( "DOWN" );
        ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
        ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
        ctxt.register_action( "RIGHT" );
        ctxt.register_action( "LEFT" );
        ctxt.register_action( "NEXT_TAB", to_translation( "Next page" ) );
        ctxt.register_action( "PREV_TAB", to_translation( "Previous page" ) );
        ctxt.register_action( "SCROLL_ITEM_INFO_UP" );
        ctxt.register_action( "SCROLL_ITEM_INFO_DOWN" );
        ctxt.register_action( "CONFIRM" );
        ctxt.register_action( "SELECT_ALL" );
        ctxt.register_action( "QUIT", to_translation( "Cancel" ) );
        ctxt.register_action( "ANY_INPUT" );
        ctxt.register_action( "HELP_KEYBINDINGS" );
        ctxt.register_action( "FILTER" );
        ctxt.register_action( "SELECT" );
#if defined(__ANDROID__)
        ctxt.allow_text_entry = true; // allow user to specify pickup amount
#endif

        bool update = true;
        int iScrollPos = 0;

        std::string filter;
        std::string new_filter;
        // Indexes of items that match the filter
        std::vector<int> matches;
        bool filter_changed = true;

        units::mass weight_predict = 0_gram;
        units::volume volume_predict = 0_ml;
        units::length length_predict = 0_mm;
        units::volume ind_vol_predict = 0_ml;

        const std::string all_pickup_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:;";

        ui.on_redraw( [&]( const ui_adaptor & ) {
            const item &selected_item = *stacked_here[matches[selected]].front();

            if( selected >= 0 && selected <= static_cast<int>( stacked_here.size() ) - 1 ) {
                std::vector<iteminfo> vThisItem;
                selected_item.info( true, vThisItem );

                item_info_data dummy( {}, {}, vThisItem, {}, iScrollPos );
                dummy.without_getch = true;
                dummy.without_border = true;

                draw_item_info( w_item_info, dummy );
            } else {
                werase( w_item_info );
                wnoutrefresh( w_item_info );
            }
            draw_custom_border( w_item_info, 0 );

            // print info window title: < item name >
            mvwprintw( w_item_info, point( 2, 0 ), "< " );
            trim_and_print( w_item_info, point( 4, 0 ), pickupW - 8, selected_item.color_in_inventory(),
                            selected_item.display_name() );
            wprintw( w_item_info, " >" );
            wnoutrefresh( w_item_info );

            const std::string pickup_chars = ctxt.get_available_single_char_hotkeys( all_pickup_chars );

            werase( w_pickup );
            pickup_rect::list.clear();
            for( int cur_it = start; cur_it < start + maxitems; cur_it++ ) {
                if( cur_it < static_cast<int>( matches.size() ) ) {
                    int true_it = matches[cur_it];
                    const item &this_item = *stacked_here[true_it].front();
                    nc_color icolor = this_item.color_in_inventory();
                    if( cur_it == selected ) {
                        icolor = hilite( c_white );
                    }

                    if( cur_it < static_cast<int>( pickup_chars.size() ) ) {
                        mvwputch( w_pickup, point( 0, 2 + ( cur_it % maxitems ) ), icolor,
                                  static_cast<char>( pickup_chars[cur_it] ) );
                    } else if( cur_it < static_cast<int>( pickup_chars.size() ) + static_cast<int>
                               ( pickup_chars.size() ) *
                               static_cast<int>( pickup_chars.size() ) ) {
                        int p = cur_it - pickup_chars.size();
                        int p1 = p / pickup_chars.size();
                        int p2 = p % pickup_chars.size();
                        mvwprintz( w_pickup, point( 0, 2 + ( cur_it % maxitems ) ), icolor, "`%c%c",
                                   static_cast<char>( pickup_chars[p1] ), static_cast<char>( pickup_chars[p2] ) );
                    } else {
                        mvwputch( w_pickup, point( 0, 2 + ( cur_it % maxitems ) ), icolor, ' ' );
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
                    if( stacked_here[true_it].front()->is_money() ) {
                        //Count charges
                        // TODO: transition to the item_location system used for the inventory
                        unsigned int charges_total = 0;
                        for( const item_stack::iterator &it : stacked_here[true_it] ) {
                            charges_total += it->ammo_remaining();
                        }
                        //Picking up none or all the cards in a stack
                        if( !getitem[true_it].pick || getitem[true_it].count == 0 ) {
                            item_name = stacked_here[true_it].front()->display_money( stacked_here[true_it].size(),
                                        charges_total );
                        } else {
                            unsigned int charges = 0;
                            int c = getitem[true_it].count;
                            for( std::list<item_stack::iterator>::iterator it = stacked_here[true_it].begin();
                                 it != stacked_here[true_it].end() && c > 0; ++it, --c ) {
                                charges += ( *it )->ammo_remaining();
                            }
                            item_name = stacked_here[true_it].front()->display_money( getitem[true_it].count, charges_total,
                                        charges );
                        }
                    } else {
                        item_name = this_item.display_name( stacked_here[true_it].size() );
                    }
                    if( stacked_here[true_it].size() > 1 ) {
                        item_name = string_format( "%d %s", stacked_here[true_it].size(), item_name );
                    }
                    if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
                        item_name = string_format( "%s %s", this_item.symbol().c_str(),
                                                   item_name );
                    }

                    // if the item does not belong to your fraction then add the stolen symbol
                    if( !this_item.is_owned_by( player_character, true ) ) {
                        item_name = string_format( "<color_light_red>!</color> %s", item_name );
                    }

                    int y = 2 + ( cur_it % maxitems );
                    trim_and_print( w_pickup, point( 6, y ), pickupW - 6, icolor, item_name );
                    pickup_rect rect = pickup_rect( point( 6, y ), point( pickupW - 1, y ) );
                    rect.cur_it = cur_it;
                    pickup_rect::list.push_back( rect );
                }
            }

            mvwprintw( w_pickup, point( 0, maxitems + 2 ), _( "[%s] Unmark" ),
                       ctxt.get_desc( "LEFT", 1 ) );

            center_print( w_pickup, maxitems + 2, c_light_gray, string_format( _( "[%s] Help" ),
                          ctxt.get_desc( "HELP_KEYBINDINGS", 1 ) ) );

            right_print( w_pickup, maxitems + 2, 0, c_light_gray, string_format( _( "[%s] Mark" ),
                         ctxt.get_desc( "RIGHT", 1 ) ) );

            mvwprintw( w_pickup, point( 0, maxitems + 3 ), _( "[%s] Prev" ),
                       ctxt.get_desc( "PREV_TAB", 1 ) );

            center_print( w_pickup, maxitems + 3, c_light_gray, string_format( _( "[%s] All" ),
                          ctxt.get_desc( "SELECT_ALL", 1 ) ) );

            right_print( w_pickup, maxitems + 3, 0, c_light_gray, string_format( _( "[%s] Next" ),
                         ctxt.get_desc( "NEXT_TAB", 1 ) ) );

            const std::string fmted_weight_predict = colorize(
                        string_format( "%.1f", round_up( convert_weight( weight_predict ), 1 ) ),
                        weight_predict > player_character.weight_capacity() ? c_red : c_white );
            const std::string fmted_weight_capacity = string_format(
                        "%.1f", round_up( convert_weight( player_character.weight_capacity() ), 1 ) );
            const std::string fmted_volume_predict = colorize(
                        format_volume( volume_predict ),
                        volume_predict > player_character.volume_capacity() ? c_red : c_white );
            const std::string fmted_volume_capacity = format_volume( player_character.volume_capacity() );

            const std::string fmted_ind_volume_predict = colorize( format_volume( ind_vol_predict ),
                    ind_vol_predict > player_character.max_single_item_volume() ? c_red : c_white );
            const std::string fmted_ind_length_predict = colorize( string_format( "%.2f",
                    convert_length_cm_in( length_predict ) ),
                    length_predict > player_character.max_single_item_length() ? c_red : c_white );
            const std::string fmted_ind_volume_capac = format_volume(
                        player_character.max_single_item_volume() );
            const units::length indiv = player_character.max_single_item_length();
            const std::string fmted_ind_length_capac = string_format( "%.2f", convert_length_cm_in( indiv ) );

            trim_and_print( w_pickup, point_zero, pickupW, c_white,
                            string_format( _( "PICK Wgt %1$s/%2$s  Vol %3$s/%4$s" ),
                                           fmted_weight_predict, fmted_weight_capacity,
                                           fmted_volume_predict, fmted_volume_capacity
                                         ) );
            trim_and_print( w_pickup, point_south, pickupW, c_white,
                            string_format( _( "INDV Vol %1$s/%2$s  Lng %3$s/%4$s" ),
                                           fmted_ind_volume_predict, fmted_ind_volume_capac,
                                           fmted_ind_length_predict, fmted_ind_length_capac ) );

            wnoutrefresh( w_pickup );
        } );

        // Now print the two lists; those on the ground and about to be added to inv
        // Continue until we hit return or space
        do {
            const std::string pickup_chars = ctxt.get_available_single_char_hotkeys( all_pickup_chars );
            // -2 lines for border, -2 to preserve a line at top/bottom for context
            const int scroll_lines = catacurses::getmaxy( w_item_info ) - 4;
            int idx = -1;
            const int recmax = static_cast<int>( matches.size() );
            const int scroll_rate = recmax > 20 ? 10 : 3;

            if( action == "ANY_INPUT" &&
                raw_input_char >= '0' && raw_input_char <= '9' ) {
                int raw_input_char_value = static_cast<char>( raw_input_char ) - '0';
                itemcount *= 10;
                itemcount += raw_input_char_value;
                if( itemcount < 0 ) {
                    itemcount = 0;
                }
            } else if( action == "SELECT" ) {
                cata::optional<point> pos = ctxt.get_coordinates_text( w_pickup );
                if( pos ) {
                    if( window_contains_point_relative( w_pickup, pos.value() ) ) {
                        pickup_rect *rect = pickup_rect::find_by_coordinate( pos.value() );
                        if( rect != nullptr ) {
                            selected = rect->cur_it;
                            iScrollPos = 0;
                            idx = selected;
                        }
                    }
                }

            } else if( action == "SCROLL_ITEM_INFO_UP" ) {
                iScrollPos -= scroll_lines;
            } else if( action == "SCROLL_ITEM_INFO_DOWN" ) {
                iScrollPos += scroll_lines;
            } else if( action == "PREV_TAB" ) {
                if( start > 0 ) {
                    start -= maxitems;
                } else {
                    start = static_cast<int>( ( matches.size() - 1 ) / maxitems ) * maxitems;
                }
                selected = start;
            } else if( action == "NEXT_TAB" ) {
                if( start + maxitems < recmax ) {
                    start += maxitems;
                } else {
                    start = 0;
                }
                iScrollPos = 0;
                selected = start;
            } else if( action == "UP" ) {
                selected--;
                iScrollPos = 0;
                if( selected < 0 ) {
                    selected = matches.size() - 1;
                    start = static_cast<int>( matches.size() / maxitems ) * maxitems;
                    if( start >= recmax ) {
                        start -= maxitems;
                    }
                } else if( selected < start ) {
                    start -= maxitems;
                }
            } else if( action == "DOWN" ) {
                selected++;
                iScrollPos = 0;
                if( selected >= recmax ) {
                    selected = 0;
                    start = 0;
                } else if( selected >= start + maxitems ) {
                    start += maxitems;
                }
            } else if( action == "PAGE_DOWN" ) {
                if( selected == recmax - 1 ) {
                    selected = 0;
                    start = 0;
                } else if( selected + scroll_rate >= recmax ) {
                    selected = recmax - 1;
                    if( selected >= start + maxitems ) {
                        start += maxitems;
                    }
                } else {
                    selected += +scroll_rate;
                    iScrollPos = 0;
                    if( selected >= recmax ) {
                        selected = 0;
                        start = 0;
                    } else if( selected >= start + maxitems ) {
                        start += maxitems;
                    }
                }
            } else if( action == "PAGE_UP" ) {
                if( selected == 0 ) {
                    selected = recmax - 1;
                    start = static_cast<int>( matches.size() / maxitems ) * maxitems;
                    if( start >= recmax ) {
                        start -= maxitems;
                    }
                } else if( selected <= scroll_rate ) {
                    selected = 0;
                    start = 0;
                } else {
                    selected += -scroll_rate;
                    iScrollPos = 0;
                    if( selected < start ) {
                        start -= maxitems;
                    }
                }
            } else if( selected >= 0 && selected < recmax &&
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
            } else if( action == "SELECT_ALL" ) {
                int count = 0;
                for( int i : matches ) {
                    if( getitem[i].pick ) {
                        count++;
                    }
                    getitem[i].pick = true;
                }
                if( count == static_cast<int>( stacked_here.size() ) ) {
                    for( size_t i = 0; i < stacked_here.size(); i++ ) {
                        getitem[i].pick = false;
                    }
                }
                update = true;
            }

            if( idx >= 0 && idx < static_cast<int>( matches.size() ) ) {
                size_t true_idx = matches[idx];
                if( itemcount != 0 || getitem[true_idx].count == 0 ) {
                    const item &temp = *stacked_here[true_idx].front();
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
                    start = static_cast<int>( idx / maxitems ) * maxitems;
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
                        if( filter_func( *stacked_here[index].front() ) ) {
                            matches.push_back( index );
                        }
                    }
                    if( matches.empty() ) {
                        popup( _( "Your filter returned no results" ) );
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
            }

            if( update ) { // Update weight & volume information
                update = false;
                units::mass weight_picked_up = 0_gram;
                units::volume volume_picked_up = 0_ml;
                units::length length_picked_up = 0_mm;
                for( size_t i = 0; i < getitem.size(); i++ ) {
                    if( getitem[i].pick ) {
                        // Make a copy for calculating weight/volume
                        item temp = *stacked_here[i].front();
                        if( temp.count_by_charges() && getitem[i].count < temp.charges && getitem[i].count != 0 ) {
                            temp.charges = getitem[i].count;
                        }
                        int num_picked = std::min( stacked_here[i].size(),
                                                   getitem[i].count == 0 ? stacked_here[i].size() : getitem[i].count );
                        weight_picked_up += temp.weight() * num_picked;
                        volume_picked_up += temp.volume() * num_picked;
                        length_picked_up = temp.length();
                    }
                }

                weight_predict = player_character.weight_carried() + weight_picked_up;
                volume_predict = player_character.volume_carried() + volume_picked_up;
                ind_vol_predict = volume_picked_up;
                length_predict = length_picked_up;
            }

            ui_manager::redraw();
            action = ctxt.handle_input();
            raw_input_char = ctxt.get_raw_input().get_first_input();

        } while( action != "QUIT" && action != "CONFIRM" );

        bool item_selected = false;
        // Check if we have selected an item.
        for( pickup_count selection : getitem ) {
            if( selection.pick ) {
                item_selected = true;
            }
        }
        if( action != "CONFIRM" || !item_selected ) {
            add_msg( _( "Never mind." ) );
            g->reenter_fullscreen();
            return;
        }
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

    std::vector<item_location> target_items;
    std::vector<int> quantities;
    for( std::pair<item_stack::iterator, int> &iter_qty : pick_values ) {
        if( from_vehicle ) {
            target_items.emplace_back( vehicle_cursor( *veh, cargo_part ), &*iter_qty.first );
        } else {
            target_items.emplace_back( map_cursor( p ), &*iter_qty.first );
        }
        quantities.push_back( iter_qty.second );
    }

    player_character.assign_activity( player_activity( pickup_activity_actor( target_items, quantities,
                                      player_character.pos() ) ) );
    if( min == -1 ) {
        // Auto pickup will need to auto resume since there can be several of them on the stack.
        player_character.activity.auto_resume = true;
    }

    g->reenter_fullscreen();
}

//helper function for Pickup::pick_up
void show_pickup_message( const PickupMap &mapPickup )
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
