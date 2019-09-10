#include "iexamine.h"

#include <climits>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>

#include "ammo.h"
#include "avatar.h"
#include "basecamp.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "compatibility.h" // needed for the workaround for the std::to_string bug in some compilers
#include "construction.h"
#include "coordinate_conversions.h"
#include "craft_command.h"
#include "debug.h"
#include "effect.h"
#include "event_bus.h"
#include "timed_event.h"
#include "field.h"
#include "fungal_effects.h"
#include "game.h"
#include "game_inventory.h"
#include "handle_liquid.h"
#include "harvest.h"
#include "input.h"
#include "inventory.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "material.h"
#include "messages.h"
#include "mission_companion.h"
#include "monster.h"
#include "mtype.h"
#include "iuse_actor.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pickup.h"
#include "player.h"
#include "recipe.h"
#include "requirements.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "uistate.h"
#include "units.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "weather.h"
#include "bodypart.h"
#include "color.h"
#include "creature.h"
#include "cursesdef.h"
#include "damage.h"
#include "enums.h"
#include "game_constants.h"
#include "int_id.h"
#include "item.h"
#include "item_location.h"
#include "item_stack.h"
#include "iuse.h"
#include "map_selector.h"
#include "pimpl.h"
#include "player_activity.h"
#include "pldata.h"
#include "string_id.h"
#include "colony.h"
#include "flat_set.h"
#include "magic_teleporter_list.h"
#include "point.h"

const mtype_id mon_dark_wyrm( "mon_dark_wyrm" );
const mtype_id mon_fungal_blossom( "mon_fungal_blossom" );
const mtype_id mon_spider_web_s( "mon_spider_web_s" );
const mtype_id mon_spider_widow_giant_s( "mon_spider_widow_giant_s" );
const mtype_id mon_spider_cellar_giant_s( "mon_spider_cellar_giant_s" );
const mtype_id mon_turret_rifle( "mon_turret_rifle" );

const skill_id skill_computer( "computer" );
const skill_id skill_fabrication( "fabrication" );
const skill_id skill_electronics( "electronics" );
const skill_id skill_firstaid( "firstaid" );
const skill_id skill_mechanics( "mechanics" );
const skill_id skill_cooking( "cooking" );
const skill_id skill_survival( "survival" );

const efftype_id effect_mending( "mending" );
const efftype_id effect_pkill2( "pkill2" );
const efftype_id effect_teleglow( "teleglow" );
const efftype_id effect_sleep( "sleep" );

static const trait_id trait_AMORPHOUS( "AMORPHOUS" );
static const trait_id trait_ARACHNID_ARMS_OK( "ARACHNID_ARMS_OK" );
static const trait_id trait_BADKNEES( "BADKNEES" );
static const trait_id trait_BURROW( "BURROW" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_INSECT_ARMS_OK( "INSECT_ARMS_OK" );
static const trait_id trait_M_DEFENDER( "M_DEFENDER" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_M_FERTILE( "M_FERTILE" );
static const trait_id trait_M_SPORES( "M_SPORES" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PARKOUR( "PARKOUR" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_THRESH_MARLOSS( "THRESH_MARLOSS" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );
const trap_str_id tr_unfinished_construction( "tr_unfinished_construction" );

const zone_type_id z_loot_unsorted( "LOOT_UNSORTED" );

static void pick_plant( player &p, const tripoint &examp, const std::string &itemType,
                        ter_id new_ter,
                        bool seeds = false );

/**
 * Nothing player can interact with here.
 */
void iexamine::none( player &p, const tripoint &examp )
{
    ( void )p; //unused
    add_msg( _( "That is a %s." ), g->m.name( examp ) );
}

/**
 * Pick an appropriate item and apply diamond coating if possible.
 */
void iexamine::cvdmachine( player &p, const tripoint & )
{
    // Select an item to which it is possible to apply a diamond coating
    auto loc = g->inv_map_splice( []( const item & e ) {
        return ( e.is_melee( DT_CUT ) || e.is_melee( DT_STAB ) ) && e.made_of( material_id( "steel" ) ) &&
               !e.has_flag( "DIAMOND" ) && !e.has_flag( "NO_CVD" );
    }, _( "Apply diamond coating" ), 1, _( "You don't have a suitable item to coat with diamond" ) );

    if( !loc ) {
        return;
    }

    // Require materials proportional to selected item volume
    auto qty = loc->volume() / units::legacy_volume_factor;
    qty = std::max( 1, qty );
    auto reqs = *requirement_id( "cvd_diamond" ) * qty;

    if( !reqs.can_make_with_inventory( p.crafting_inventory(), is_crafting_component ) ) {
        popup( "%s", reqs.list_missing() );
        return;
    }

    // Consume materials
    for( const auto &e : reqs.get_components() ) {
        p.consume_items( e, 1, is_crafting_component );
    }
    for( const auto &e : reqs.get_tools() ) {
        p.consume_tools( e );
    }
    p.invalidate_crafting_inventory();

    // Apply flag to item
    loc->item_tags.insert( "DIAMOND" );
    add_msg( m_good, _( "You apply a diamond coating to your %s" ), loc->type_name() );
    p.mod_moves( -to_turns<int>( 10_seconds ) );
}

/**
 * UI FOR LAB_FINALE NANO FABRICATOR.
 */
void iexamine::nanofab( player &p, const tripoint &examp )
{
    bool table_exists = false;
    tripoint spawn_point;
    for( const auto &valid_location : g->m.points_in_radius( examp, 1, 0 ) ) {
        if( g->m.ter( valid_location ) == ter_str_id( "t_nanofab_body" ) ) {
            spawn_point = valid_location;
            table_exists = true;
            break;
        }
    }
    if( !table_exists ) {
        return;
    }

    auto nanofab_template = g->inv_map_splice( []( const item & e ) {
        return e.has_var( "NANOFAB_ITEM_ID" );
    }, _( "Introduce Nanofabricator template" ), PICKUP_RANGE,
    _( "You don't have any usable templates." ) );

    if( !nanofab_template ) {
        return;
    }

    item new_item( nanofab_template->get_var( "NANOFAB_ITEM_ID" ), calendar::turn );

    auto qty = std::max( 1, new_item.volume() / 250_ml );
    auto reqs = *requirement_id( "nanofabricator" ) * qty;

    if( !reqs.can_make_with_inventory( p.crafting_inventory(), is_crafting_component ) ) {
        popup( "%s", reqs.list_missing() );
        return;
    }

    // Consume materials
    for( const auto &e : reqs.get_components() ) {
        p.consume_items( e, 1, is_crafting_component );
    }
    for( const auto &e : reqs.get_tools() ) {
        p.consume_tools( e );
    }
    p.invalidate_crafting_inventory();

    if( new_item.is_armor() && new_item.has_flag( "VARSIZE" ) ) {
        new_item.item_tags.insert( "FIT" );
    }

    g->m.add_item_or_charges( spawn_point, new_item );

}

/**
 * Use "gas pump."  Will pump any liquids on tile.
 */
void iexamine::gaspump( player &p, const tripoint &examp )
{
    if( !query_yn( _( "Use the %s?" ), g->m.tername( examp ) ) ) {
        none( p, examp );
        return;
    }

    auto items = g->m.i_at( examp );
    for( auto item_it = items.begin(); item_it != items.end(); ++item_it ) {
        if( item_it->made_of( LIQUID ) ) {
            ///\EFFECT_DEX decreases chance of spilling gas from a pump
            if( one_in( 10 + p.get_dex() ) ) {
                add_msg( m_bad, _( "You accidentally spill the %s." ), item_it->type_name() );
                static const auto max_spill_volume = units::from_liter( 1 );
                const int max_spill_charges = std::max( 1, item_it->charges_per_volume( max_spill_volume ) );
                ///\EFFECT_DEX decreases amount of gas spilled from a pump
                const int qty = rng( 1, max_spill_charges * 8.0 / std::max( 1, p.get_dex() ) );

                item spill = item_it->split( qty );
                if( spill.is_null() ) {
                    g->m.add_item_or_charges( p.pos(), *item_it );
                    items.erase( item_it );
                } else {
                    g->m.add_item_or_charges( p.pos(), spill );
                }

            } else {
                liquid_handler::handle_liquid_from_ground( item_it, examp, 1 );
            }
            return;
        }
    }
    add_msg( m_info, _( "Out of order." ) );
}

void iexamine::translocator( player &, const tripoint &examp )
{
    const tripoint omt_loc = ms_to_omt_copy( g->m.getabs( examp ) );
    const bool activated = g->u.translocators.knows_translocator( examp );
    if( !activated ) {
        g->u.translocators.activate_teleporter( omt_loc, examp );
        add_msg( m_info, _( "Translocator gate active." ) );
    } else {
        if( query_yn( _( "Do you want to deactivate this active Translocator?" ) ) ) {
            g->u.translocators.deactivate_teleporter( omt_loc, examp );
        }
    }
}

namespace
{
//--------------------------------------------------------------------------------------------------
//! Implements iexamine::atm(...)
//--------------------------------------------------------------------------------------------------
class atm_menu
{
    public:
        // menu choices
        enum options : int {
            cancel, purchase_card, deposit_money, withdraw_money, transfer_all_money
        };

        atm_menu()                           = delete;
        atm_menu( atm_menu const & )            = delete;
        atm_menu( atm_menu && )                 = delete;
        atm_menu &operator=( atm_menu const & ) = delete;
        atm_menu &operator=( atm_menu && )      = delete;

        explicit atm_menu( player &p ) : u( p ) {
            reset( false );
        }

        void start() {
            for( bool result = false; !result; ) {
                switch( choose_option() ) {
                    case purchase_card:
                        result = do_purchase_card();
                        break;
                    case deposit_money:
                        result = do_deposit_money();
                        break;
                    case withdraw_money:
                        result = do_withdraw_money();
                        break;
                    case transfer_all_money:
                        result = do_transfer_all_money();
                        break;
                    default:
                        return;
                }
                if( !u.activity.is_null() ) {
                    break;
                }
                g->draw();
            }
        }
    private:
        void add_choice( const int i, const char *const title ) {
            amenu.addentry( i, true, -1, title );
        }
        void add_info( const int i, const char *const title ) {
            amenu.addentry( i, false, -1, title );
        }

        options choose_option() {
            if( u.activity.id() == activity_id( "ACT_ATM" ) ) {
                return static_cast<options>( u.activity.index );
            }
            amenu.query();
            uistate.iexamine_atm_selected = amenu.selected;
            return amenu.ret < 0 ? cancel : static_cast<options>( amenu.ret );
        }

        //! Reset and repopulate the menu; with a fair bit of work this could be more efficient.
        void reset( const bool clear = true ) {
            const int card_count   = u.amount_of( "cash_card" );
            const int charge_count = card_count ? u.charges_of( "cash_card" ) : 0;

            if( clear ) {
                amenu.reset();
            }

            amenu.selected = uistate.iexamine_atm_selected;
            amenu.text = string_format( _( "Welcome to the C.C.B.o.t.T. ATM. What would you like to do?\n"
                                           "Your current balance is: %s" ),
                                        format_money( u.cash ) );

            if( u.cash >= 100 ) {
                add_choice( purchase_card, _( "Purchase cash card?" ) );
            } else {
                add_info( purchase_card, _( "You need $1.00 in your account to purchase a card." ) );
            }

            if( card_count && u.cash > 0 ) {
                add_choice( withdraw_money, _( "Withdraw Money" ) );
            } else if( u.cash > 0 ) {
                add_info( withdraw_money, _( "You need a cash card before you can withdraw money!" ) );
            } else if( u.cash < 0 ) {
                add_info( withdraw_money,
                          _( "You need to pay down your debt first!" ) );
            } else {
                add_info( withdraw_money,
                          _( "You need money in your account before you can withdraw money!" ) );
            }

            if( charge_count ) {
                add_choice( deposit_money, _( "Deposit Money" ) );
            } else {
                add_info( deposit_money,
                          _( "You need a charged cash card before you can deposit money!" ) );
            }

            if( card_count >= 2 && charge_count ) {
                add_choice( transfer_all_money, _( "Transfer All Money" ) );
            }
        }

        //! print a bank statement for @p print = true;
        void finish_interaction( const bool print = true ) {
            if( print ) {
                if( u.cash < 0 ) {
                    add_msg( m_info, _( "Your debt is now %s." ), format_money( u.cash ) );
                } else {
                    add_msg( m_info, _( "Your account now holds %s." ), format_money( u.cash ) );
                }
            }

            u.moves -= to_turns<int>( 5_seconds );
        }

        //! Prompt for an integral value clamped to [0, max].
        static int prompt_for_amount( const char *const msg, const int max ) {
            const std::string formatted = string_format( msg, max );
            const int amount = string_input_popup()
                               .title( formatted )
                               .width( 20 )
                               .text( to_string( max ) )
                               .only_digits( true )
                               .query_int();

            return ( amount > max ) ? max : ( amount <= 0 ) ? 0 : amount;
        }

        //!Get a new cash card. $1.00 fine.
        bool do_purchase_card() {
            const char *prompt = _( "This will automatically deduct $1.00 from your bank account. Continue?" );

            if( !query_yn( prompt ) ) {
                return false;
            }

            item card( "cash_card", calendar::turn );
            card.charges = 0;
            u.i_add( card );
            u.cash -= 100;
            u.moves -= to_turns<int>( 5_seconds );
            finish_interaction();

            return true;
        }

        //!Deposit money from cash card into bank account.
        bool do_deposit_money() {
            int money = u.charges_of( "cash_card" );

            if( !money ) {
                popup( _( "You can only deposit money from charged cash cards!" ) );
                return false;
            }

            const int amount = prompt_for_amount( ngettext(
                    "Deposit how much? Max: %d cent. (0 to cancel) ",
                    "Deposit how much? Max: %d cents. (0 to cancel) ", money ), money );

            if( !amount ) {
                return false;
            }

            add_msg( m_info, "amount: %d", amount );
            u.use_charges( "cash_card", amount );
            u.cash += amount;
            u.moves -= to_turns<int>( 10_seconds );
            finish_interaction();

            return true;
        }

        //!Move money from bank account onto cash card.
        bool do_withdraw_money() {
            //We may want to use visit_items here but thats fairly heavy.
            //For now, just check weapon if we didnt find it in the inventory.
            int pos = u.inv.position_by_type( "cash_card" );
            item *dst;
            if( pos == INT_MIN ) {
                dst = &u.weapon;
            } else {
                dst = &u.i_at( pos );
            }

            if( dst->is_null() ) {
                //Just in case we run into an edge case
                popup( _( "You do not have a cash card to withdraw money!" ) );
                return false;
            }

            const int amount = prompt_for_amount( ngettext(
                    "Withdraw how much? Max: %d cent. (0 to cancel) ",
                    "Withdraw how much? Max: %d cents. (0 to cancel) ", u.cash ), u.cash );

            if( !amount ) {
                return false;
            }

            dst->charges += amount;
            u.cash -= amount;
            u.moves -= to_turns<int>( 10_seconds );
            finish_interaction();

            return true;
        }

        //!Move the money from all the cash cards in inventory to a single card.
        bool do_transfer_all_money() {
            item *dst;
            if( u.activity.id() == activity_id( "ACT_ATM" ) ) {
                u.activity.set_to_null(); // stop for now, if required, it will be created again.
                dst = &u.i_at( u.activity.position );
                if( dst->is_null() || dst->typeId() != "cash_card" ) {
                    return false;
                }
            } else {
                const int pos = u.inv.position_by_type( "cash_card" );

                if( pos == INT_MIN ) {
                    return false;
                }
                dst = &u.i_at( pos );
            }

            for( auto &i : u.inv_dump() ) {
                if( i == dst || i->charges <= 0 || i->typeId() != "cash_card" ) {
                    continue;
                }
                if( u.moves < 0 ) {
                    // Money from `*i` could be transferred, but we're out of moves, schedule it for
                    // the next turn. Putting this here makes sure there will be something to be
                    // done next turn.
                    u.assign_activity( activity_id( "ACT_ATM" ), 0, transfer_all_money, u.get_item_position( dst ) );
                    break;
                }

                dst->charges += i->charges;
                i->charges = 0;
                u.moves -= 10;
            }

            return true;
        }

        player &u;
        uilist amenu;
};
} //namespace

/**
 * launches the atm menu class which then handles all the atm interactions.
 */
void iexamine::atm( player &p, const tripoint & )
{
    atm_menu {p} .start();
}

/**
 * Generates vending machine UI and allows players to purchase contained items with a cash card.
 */
void iexamine::vending( player &p, const tripoint &examp )
{
    constexpr int moves_cost = to_turns<int>( 5_seconds );
    int money = p.charges_of( "cash_card" );
    auto vend_items = g->m.i_at( examp );

    if( vend_items.empty() ) {
        add_msg( m_info, _( "The vending machine is empty!" ) );
        return;
    } else if( !money ) {
        popup( _( "You need a charged cash card to purchase things!" ) );
        return;
    }

    const int padding_x  = std::max( 0, TERMX - FULL_SCREEN_WIDTH ) / 4;
    const int padding_y  = std::max( 0, TERMY - FULL_SCREEN_HEIGHT ) / 6;
    const int window_h   = FULL_SCREEN_HEIGHT + std::max( 0, TERMY - FULL_SCREEN_HEIGHT ) * 2 / 3;
    const int window_w   = FULL_SCREEN_WIDTH + std::max( 0, TERMX - FULL_SCREEN_WIDTH ) / 2;
    const int w_items_w  = window_w / 2;
    const int w_info_w   = window_w - w_items_w;
    const int list_lines = window_h - 4; // minus for header and footer

    constexpr int first_item_offset = 3; // header size

    catacurses::window const w = catacurses::newwin( window_h, w_items_w, point( padding_x,
                                 padding_y ) );
    catacurses::window const w_item_info = catacurses::newwin( window_h, w_info_w,
                                           point( padding_x + w_items_w, padding_y ) );

    bool used_machine = false;
    input_context ctxt( "VENDING_MACHINE" );
    ctxt.register_updown();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    // Collate identical items.
    // First, build a map {item::tname} => {item_it, item_it, item_it...}
    using iterator_t = decltype( std::begin( vend_items ) ); // map_stack::iterator doesn't exist.

    std::map<std::string, std::vector<iterator_t>> item_map;
    for( auto it = std::begin( vend_items ); it != std::end( vend_items ); ++it ) {
        // |# {name}|
        // 123      4
        item_map[utf8_truncate( it->tname(), static_cast<size_t>( w_items_w - 4 ) )].push_back( it );
    }

    // Next, put pointers to the pairs in the map in a vector to allow indexing.
    std::vector<std::map<std::string, std::vector<iterator_t>>::value_type *> item_list;
    item_list.reserve( item_map.size() );
    for( auto &pair : item_map ) {
        item_list.emplace_back( &pair );
    }

    const int lines_above = list_lines / 2;                  // lines above the selector
    const int lines_below = list_lines / 2 + list_lines % 2; // lines below the selector

    int cur_pos = 0;
    for( ;; ) {
        const int num_items = item_list.size();
        const int page_size = std::min( num_items, list_lines );

        werase( w );
        wborder( w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
        mvwhline( w, point( 1, first_item_offset - 1 ), LINE_OXOX, w_items_w - 2 );
        mvwaddch( w, point( 0, first_item_offset - 1 ), LINE_XXXO ); // |-
        mvwaddch( w, point( w_items_w - 1, first_item_offset - 1 ), LINE_XOXX ); // -|

        trim_and_print( w, point( 2, 1 ), w_items_w - 3, c_light_gray,
                        _( "Money left: %s" ), format_money( money ) );

        // Keep the item selector centered in the page.
        int page_beg;
        if( cur_pos < num_items - cur_pos ) {
            page_beg = std::max( 0, cur_pos - lines_above );
        } else {
            int page_end = std::min( num_items, cur_pos + lines_below );
            page_beg = std::max( 0, page_end - list_lines );
        }

        for( int line = 0; line < page_size; ++line ) {
            const int i = page_beg + line;
            const auto color = ( i == cur_pos ) ? h_light_gray : c_light_gray;
            const auto &elem = item_list[i];
            const int count = elem->second.size();
            const char c = ( count < 10 ) ? ( '0' + count ) : '*';
            trim_and_print( w, point( 1, first_item_offset + line ), w_items_w - 3, color, "%c %s", c,
                            elem->first.c_str() );
        }

        draw_scrollbar( w, cur_pos, list_lines, num_items, point( 0, first_item_offset ) );
        wrefresh( w );

        // Item info
        auto &cur_items = item_list[static_cast<size_t>( cur_pos )]->second;
        auto &cur_item  = cur_items.back();

        werase( w_item_info );
        // | {line}|
        // 12      3
        fold_and_print( w_item_info, point( 2, 1 ), w_info_w - 3, c_light_gray, cur_item->info( true ) );
        wborder( w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

        //+<{name}>+
        //12      34
        const std::string name = utf8_truncate( cur_item->display_name(),
                                                static_cast<size_t>( w_info_w - 4 ) );
        mvwprintw( w_item_info, point_east, "<%s>", name );
        wrefresh( w_item_info );

        const std::string &action = ctxt.handle_input();
        if( action == "DOWN" ) {
            cur_pos = ( cur_pos + 1 ) % num_items;
        } else if( action == "UP" ) {
            cur_pos = ( cur_pos + num_items - 1 ) % num_items;
        } else if( action == "CONFIRM" ) {
            const int iprice = cur_item->price( false );

            if( iprice > money ) {
                popup( _( "That item is too expensive!" ) );
                continue;
            }

            if( !used_machine ) {
                used_machine = true;
                p.moves -= moves_cost;
            }

            money -= iprice;
            p.use_charges( "cash_card", iprice );
            p.i_add_or_drop( *cur_item );

            vend_items.erase( cur_item );
            cur_items.pop_back();
            if( !cur_items.empty() ) {
                continue;
            }

            item_list.erase( std::begin( item_list ) + cur_pos );
            if( item_list.empty() ) {
                add_msg( _( "With a beep, the empty vending machine shuts down" ) );
                return;
            } else if( cur_pos == num_items - 1 ) {
                cur_pos--;
            }
        } else if( action == "QUIT" ) {
            break;
        }
    }
}

/**
 * If there's water, allow its usage but add chance of poison.
 */
void iexamine::toilet( player &p, const tripoint &examp )
{
    auto items = g->m.i_at( examp );
    auto water = items.begin();
    for( ; water != items.end(); ++water ) {
        if( water->typeId() == "water" ) {
            break;
        }
    }

    if( water == items.end() ) {
        add_msg( m_info, _( "This toilet is empty." ) );
    } else if( !water->made_of( LIQUID ) ) {
        add_msg( m_info, _( "The toilet water is frozen solid!" ) );
    } else {
        // Use a different poison value each time water is drawn from the toilet.
        water->poison = one_in( 3 ) ? 0 : rng( 1, 3 );

        ( void ) p; // TODO: use me
        liquid_handler::handle_liquid_from_ground( water, examp );
    }
}

/**
 * If underground, move 2 levels up else move 2 levels down. Stable movement between levels 0 and -2.
 */
void iexamine::elevator( player &p, const tripoint &examp )
{
    ( void )p; //unused
    if( !query_yn( _( "Use the %s?" ), g->m.tername( examp ) ) ) {
        return;
    }
    int movez = ( examp.z < 0 ? 2 : -2 );
    g->vertical_move( movez, false );
}

/**
 * Open gate.
 */
void iexamine::controls_gate( player &p, const tripoint &examp )
{
    if( !query_yn( _( "Use the %s?" ), g->m.tername( examp ) ) ) {
        none( p, examp );
        return;
    }
    g->open_gate( examp );
}

/**
 * Use id/hack reader. Using an id despawns turrets.
 */
void iexamine::cardreader( player &p, const tripoint &examp )
{
    bool open = false;
    itype_id card_type = ( g->m.ter( examp ) == t_card_science ? "id_science" :
                           g->m.ter( examp ) == t_card_military ? "id_military" : "id_industrial" );
    if( p.has_amount( card_type, 1 ) && query_yn( _( "Swipe your ID card?" ) ) ) {
        p.mod_moves( -to_turns<int>( 1_seconds ) );
        for( const tripoint &tmp : g->m.points_in_radius( examp, 3 ) ) {
            if( g->m.ter( tmp ) == t_door_metal_locked ) {
                g->m.ter_set( tmp, t_floor );
                open = true;
            }
        }
        for( monster &critter : g->all_monsters() ) {
            // Check 1) same overmap coords, 2) turret, 3) hostile
            if( ms_to_omt_copy( g->m.getabs( critter.pos() ) ) == ms_to_omt_copy( g->m.getabs( examp ) ) &&
                ( critter.type->id == mon_turret_rifle ) &&
                critter.attitude_to( p ) == Creature::Attitude::A_HOSTILE ) {
                g->remove_zombie( critter );
            }
        }
        if( open ) {
            add_msg( _( "You insert your ID card." ) );
            add_msg( m_good, _( "The nearby doors slide into the floor." ) );
            p.use_amount( card_type, 1 );
        } else {
            add_msg( _( "The nearby doors are already opened." ) );
        }
    } else {
        p.assign_activity( activity_id( "ACT_HACK_DOOR" ), to_moves<int>( 5_minutes ) );
        p.activity.placement = examp;
    }
}

void iexamine::cardreader_robofac( player &p, const tripoint &examp )
{
    itype_id card_type = "id_science";
    if( p.has_amount( card_type, 1 ) && query_yn( _( "Swipe your ID card?" ) ) ) {
        p.mod_moves( -100 );
        p.use_amount( card_type, 1 );
        add_msg( m_bad, _( "The card reader short circuits!" ) );
        g->m.ter_set( examp, t_card_reader_broken );
        intercom( p, examp );
    } else {
        add_msg( _( "You have never seen this card reader model before.  Hacking it seems impossible." ) );
    }
}

void iexamine::cardreader_foodplace( player &p, const tripoint &examp )
{
    bool open = false;
    if( ( p.is_wearing( itype_id( "foodperson_mask" ) ) ||
          p.is_wearing( itype_id( "foodperson_mask_on" ) ) ) &&
        query_yn( _( "Press mask on the reader?" ) ) ) {
        p.mod_moves( -100 );
        for( const tripoint &tmp : g->m.points_in_radius( examp, 3 ) ) {
            if( g->m.ter( tmp ) == t_door_metal_locked ) {
                g->m.ter_set( tmp, t_door_metal_c );
                open = true;
            }
        }
        if( open ) {
            add_msg( _( "You press your face on the reader." ) );
            add_msg( m_good, _( "The nearby doors are unlocked." ) );
            sounds::sound( examp, 6, sounds::sound_t::speech, _( "\"Hello Foodperson.  Welcome home.\"" ), true,
                           "speech", "welcome" );
        } else {
            add_msg( _( "The nearby doors are already unlocked." ) );
            if( query_yn( _( "Lock doors?" ) ) ) {
                for( const tripoint &tmp : g->m.points_in_radius( examp, 3 ) ) {
                    if( g->m.ter( tmp ) == t_door_metal_o || g->m.ter( tmp ) == t_door_metal_c ) {
                        if( p.pos() == tmp ) {
                            p.add_msg_if_player( m_bad, _( "You are in the way of the door, move before trying again." ) );
                        } else {
                            g->m.ter_set( tmp, t_door_metal_locked );
                        }
                    }
                }
            }
        }
    } else if( p.has_amount( itype_id( "foodperson_mask" ), 1 ) ||
               p.has_amount( itype_id( "foodperson_mask_on" ), 1 ) ) {
        sounds::sound( examp, 6, sounds::sound_t::speech,
                       _( "\"FOODPERSON DETECTED.  Please make yourself presentable.\"" ), true,
                       "speech", "welcome" );
    } else {
        sounds::sound( examp, 6, sounds::sound_t::speech,
                       _( "\"Your face is inadequate.  Please go away.\"" ), true,
                       "speech", "welcome" );
        p.assign_activity( activity_id( "ACT_HACK_DOOR" ), to_moves<int>( 5_minutes ) );
        p.activity.placement = examp;
    }
}

void iexamine::intercom( player &p, const tripoint &examp )
{
    const std::vector<npc *> intercom_npcs = g->get_npcs_if( [examp]( const npc & guy ) {
        return guy.name == "the intercom" && rl_dist( guy.pos(), examp ) < 10;
    } );
    if( intercom_npcs.empty() ) {
        p.add_msg_if_player( m_info, _( "No one responds." ) );
    } else {
        intercom_npcs.front()->talk_to_u( false, false );
    }
}

/**
 * Prompt removal of rubble. Select best shovel and invoke "CLEAR_RUBBLE" on tile.
 */
void iexamine::rubble( player &p, const tripoint &examp )
{
    int moves;
    if( p.has_quality( quality_id( "DIG" ), 3 ) || p.has_trait( trait_BURROW ) ) {
        moves = to_moves<int>( 1_minutes );
    } else if( p.has_quality( quality_id( "DIG" ), 2 ) ) {
        moves = to_moves<int>( 2_minutes );
    } else {
        add_msg( m_info, _( "If only you had a shovel..." ) );
        return;
    }
    if( ( g->m.veh_at( examp ) || !g->m.tr_at( examp ).is_null() ||
          g->critter_at( examp ) != nullptr ) &&
        !query_yn( _( "Clear up that %s?" ), g->m.furnname( examp ) ) ) {
        return;
    }
    p.assign_activity( activity_id( "ACT_CLEAR_RUBBLE" ), moves, -1, 0 );
    p.activity.placement = examp;
    return;
}

/**
 * Prompt climbing over fence. Calculates move cost, applies it to player and, moves them.
 */
void iexamine::chainfence( player &p, const tripoint &examp )
{
    // Skip prompt if easy to climb.
    if( !g->m.has_flag( "CLIMB_SIMPLE", examp ) ) {
        if( !query_yn( _( "Climb %s?" ), g->m.tername( examp ) ) ) {
            none( p, examp );
            return;
        }
    }
    if( g->m.has_flag( "CLIMB_SIMPLE", examp ) && p.has_trait( trait_PARKOUR ) ) {
        add_msg( _( "You vault over the obstacle with ease." ) );
        p.moves -= 100; // Not tall enough to warrant spider-climbing, so only relevant trait.
    } else if( g->m.has_flag( "CLIMB_SIMPLE", examp ) ) {
        add_msg( _( "You vault over the obstacle." ) );
        p.moves -= 300; // Most common move cost for barricades pre-change.
    } else if( p.has_trait( trait_ARACHNID_ARMS_OK ) && !p.wearing_something_on( bp_torso ) ) {
        add_msg( _( "Climbing this obstacle is trivial for one such as you." ) );
        p.moves -= 75; // Yes, faster than walking.  6-8 limbs are impressive.
    } else if( p.has_trait( trait_INSECT_ARMS_OK ) && !p.wearing_something_on( bp_torso ) ) {
        add_msg( _( "You quickly scale the fence." ) );
        p.moves -= 90;
    } else if( p.has_trait( trait_PARKOUR ) ) {
        add_msg( _( "This obstacle is no match for your freerunning abilities." ) );
        p.moves -= 100;
    } else {
        p.moves -= 400;
        ///\EFFECT_DEX decreases chances of slipping while climbing
        int climb = p.dex_cur;
        if( p.has_trait( trait_BADKNEES ) ) {
            climb = climb / 2;
        }
        if( one_in( climb ) ) {
            add_msg( m_bad, _( "You slip while climbing and fall down again." ) );
            if( climb <= 1 ) {
                add_msg( m_bad, _( "Climbing this obstacle is impossible in your current state." ) );
            }
            return;
        }
        p.moves += climb * 10;
        sfx::play_variant_sound( "plmove", "clear_obstacle", sfx::get_heard_volume( g->u.pos() ) );
    }
    if( p.in_vehicle ) {
        g->m.unboard_vehicle( p.pos() );
    }
    p.setpos( examp );
    if( examp.x < HALF_MAPSIZE_X || examp.y < HALF_MAPSIZE_Y ||
        examp.x >= HALF_MAPSIZE_X + SEEX || examp.y >= HALF_MAPSIZE_Y + SEEY ) {
        if( p.is_player() ) {
            g->update_map( p );
        }
    }
}

/**
 * If player has amorphous trait, slip through the bars.
 */
void iexamine::bars( player &p, const tripoint &examp )
{
    if( !( p.has_trait( trait_AMORPHOUS ) ) ) {
        none( p, examp );
        return;
    }
    if( ( ( p.encumb( bp_torso ) ) >= 10 ) && ( ( p.encumb( bp_head ) ) >= 10 ) &&
        ( p.encumb( bp_foot_l ) >= 10 ||
          p.encumb( bp_foot_r ) >= 10 ) ) { // Most likely places for rigid gear that would catch on the bars.
        add_msg( m_info,
                 _( "Your amorphous body could slip though the %s, but your cumbersome gear can't." ),
                 g->m.tername( examp ) );
        return;
    }
    if( !query_yn( _( "Slip through the %s?" ), g->m.tername( examp ) ) ) {
        none( p, examp );
        return;
    }
    p.moves -= to_turns<int>( 2_seconds );
    add_msg( _( "You slide right between the bars." ) );
    p.setpos( examp );
}

void iexamine::deployed_furniture( player &p, const tripoint &pos )
{
    if( !query_yn( _( "Take down the %s?" ), g->m.furn( pos ).obj().name() ) ) {
        return;
    }
    p.add_msg_if_player( m_info, _( "You take down the %s." ),
                         g->m.furn( pos ).obj().name() );
    const auto furn_item = g->m.furn( pos ).obj().deployed_item;
    g->m.add_item_or_charges( pos, item( furn_item, calendar::turn ) );
    g->m.furn_set( pos, f_null );
}

static std::pair<itype_id, const deploy_tent_actor *> find_tent_itype( const furn_str_id &id )
{
    const itype_id &iid = id->deployed_item;
    if( item::type_is_defined( iid ) ) {
        const itype &type = *item::find_type( iid );
        for( const auto &pair : type.use_methods ) {
            const auto actor = dynamic_cast<const deploy_tent_actor *>( pair.second.get_actor_ptr() );
            if( !actor ) {
                continue;
            }
            if( ( actor->floor_center && *actor->floor_center == id ) || ( !actor->floor_center &&
                    actor->floor == id ) ) {
                return std::make_pair( iid, actor );
            }
        }
    }
    return std::make_pair( iid, nullptr );
}

/**
 * Determine structure's type and prompts its removal.
 */
void iexamine::portable_structure( player &p, const tripoint &examp )
{
    const furn_str_id fid = g->m.furn( examp ).id();
    const std::pair<itype_id, const deploy_tent_actor *> tent_item_type = find_tent_itype( fid );
    if( tent_item_type.first == "null" ) {
        debugmsg( "unknown furniture %s: don't know how to transform it into an item", fid.str() );
        return;
    }

    itype_id dropped = tent_item_type.first;
    std::string name = item::nname( dropped );
    int radius;

    if( tent_item_type.second ) {
        const deploy_tent_actor &actor = *tent_item_type.second;
        if( !actor.check_intact( examp ) ) {
            if( !actor.broken_type ) {
                add_msg( _( "The %s is broken and can not be picked up." ), name );
                none( p, examp );
                return;
            }
            dropped = *actor.broken_type;
            name = string_format( _( "damaged %s" ), name );
        }
        radius = actor.radius;

    } else {
        radius = std::max( 1, fid->bash.collapse_radius );
    }

    if( !query_yn( _( "Take down the %s?" ), name ) ) {
        none( p, examp );
        return;
    }

    p.moves -= to_turns<int>( 2_seconds );
    for( const tripoint &pt : g->m.points_in_radius( examp, radius ) ) {
        g->m.furn_set( pt, f_null );
    }

    g->m.add_item_or_charges( examp, item( dropped, calendar::turn ) );
}

/**
 * If there is a 2x4 around, prompt placing it across pit.
 */
void iexamine::pit( player &p, const tripoint &examp )
{
    const inventory &crafting_inv = p.crafting_inventory();
    if( !crafting_inv.has_amount( "2x4", 1 ) ) {
        none( p, examp );
        return;
    }
    std::vector<item_comp> planks;
    planks.push_back( item_comp( "2x4", 1 ) );

    if( query_yn( _( "Place a plank over the pit?" ) ) ) {
        p.consume_items( planks, 1, is_crafting_component );
        if( g->m.ter( examp ) == t_pit ) {
            g->m.ter_set( examp, t_pit_covered );
        } else if( g->m.ter( examp ) == t_pit_spiked ) {
            g->m.ter_set( examp, t_pit_spiked_covered );
        } else if( g->m.ter( examp ) == t_pit_glass ) {
            g->m.ter_set( examp, t_pit_glass_covered );
        }
        add_msg( _( "You place a plank of wood over the pit." ) );
        p.mod_moves( -to_turns<int>( 1_seconds ) );
    }
}

/**
 * Prompt removing the 2x4 placed across the pit
 */
void iexamine::pit_covered( player &p, const tripoint &examp )
{
    if( !query_yn( _( "Remove cover?" ) ) ) {
        none( p, examp );
        return;
    }

    item plank( "2x4", calendar::turn );
    add_msg( _( "You remove the plank." ) );
    g->m.add_item_or_charges( p.pos(), plank );

    if( g->m.ter( examp ) == t_pit_covered ) {
        g->m.ter_set( examp, t_pit );
    } else if( g->m.ter( examp ) == t_pit_spiked_covered ) {
        g->m.ter_set( examp, t_pit_spiked );
    } else if( g->m.ter( examp ) == t_pit_glass_covered ) {
        g->m.ter_set( examp, t_pit_glass );
    }
    p.mod_moves( -to_turns<int>( 1_seconds ) );
}

/**
 * Loop prompt to bet $10.
 */
void iexamine::slot_machine( player &p, const tripoint & )
{
    if( p.cash < 1000 ) {
        add_msg( m_info, _( "You need $10 to play." ) );
    } else if( query_yn( _( "Insert $10?" ) ) ) {
        do {
            if( one_in( 5 ) ) {
                popup( _( "Three cherries... you get your money back!" ) );
            } else if( one_in( 20 ) ) {
                popup( _( "Three bells... you win $50!" ) );
                p.cash += 4000; // Minus the $10 we wagered
            } else if( one_in( 50 ) ) {
                popup( _( "Three stars... you win $200!" ) );
                p.cash += 19000;
            } else if( one_in( 1000 ) ) {
                popup( _( "JACKPOT!  You win $3000!" ) );
                p.cash += 299000;
            } else {
                popup( _( "No win." ) );
                p.cash -= 1000;
            }
        } while( p.cash >= 1000 && query_yn( _( "Play again?" ) ) );
    }
}

/**
 * Attempt to crack safe through audio-feedback manual lock manipulation.
 *
 * Try to unlock the safe by moving the dial and listening for the mechanism to "click into place."
 * Time per attempt affected by perception and mechanics. 30 minutes per attempt minimum.
 * Small chance of just guessing the combo without listening device.
 */
void iexamine::safe( player &p, const tripoint &examp )
{
    auto cracking_tool = p.crafting_inventory().items_with( []( const item & it ) -> bool {
        item temporary_item( it.type );
        return temporary_item.has_flag( "SAFECRACK" );
    } );

    if( !( !cracking_tool.empty() || p.has_bionic( bionic_id( "bio_ears" ) ) ) ) {
        p.moves -= to_turns<int>( 10_seconds );
        // one_in(30^3) chance of guessing
        if( one_in( 27000 ) ) {
            p.add_msg_if_player( m_good, _( "You mess with the dial for a little bit... and it opens!" ) );
            g->m.furn_set( examp, f_safe_o );
            return;
        } else {
            p.add_msg_if_player( m_info, _( "You mess with the dial for a little bit." ) );
            return;
        }
    }

    if( p.is_deaf() ) {
        add_msg( m_info, _( "You can't crack a safe while deaf!" ) );
        return;
    }
    if( query_yn( _( "Attempt to crack the safe?" ) ) ) {
        add_msg( m_info, _( "You start cracking the safe." ) );
        // 150 minutes +/- 20 minutes per mechanics point away from 3 +/- 10 minutes per
        // perception point away from 8; capped at 30 minutes minimum. *100 to convert to moves
        ///\EFFECT_PER speeds up safe cracking

        ///\EFFECT_MECHANICS speeds up safe cracking
        const time_duration time = std::max( 150_minutes - 20_minutes * ( p.get_skill_level(
                skill_mechanics ) - 3 ) - 10_minutes * ( p.get_per() - 8 ), 30_minutes );

        p.assign_activity( activity_id( "ACT_CRACKING" ), to_moves<int>( time ) );
        p.activity.placement = examp;
    }
}

/**
 * Attempt to pick the gunsafe's lock and open it.
 */
void iexamine::gunsafe_ml( player &p, const tripoint &examp )
{
    if( !( p.has_amount( "crude_picklock", 1 ) || p.has_amount( "hairpin", 1 ) ||
           p.has_amount( "fc_hairpin", 1 ) ||
           p.has_amount( "picklocks", 1 ) || p.has_bionic( bionic_id( "bio_lockpick" ) ) ) ) {
        add_msg( m_info, _( "You need a lockpick to open this gun safe." ) );
        return;
    } else if( !query_yn( _( "Pick the gun safe?" ) ) ) {
        return;
    }

    int pick_quality = 1;
    if( p.has_amount( "picklocks", 1 ) || p.has_bionic( bionic_id( "bio_lockpick" ) ) ) {
        pick_quality = 5;
    } else if( p.has_amount( "fc_hairpin", 1 ) ) {
        pick_quality = 1;
    } else {
        pick_quality = 3;
    }

    p.practice( skill_mechanics, 1 );

    ///\EFFECT_DEX speeds up lock picking gun safe
    ///\EFFECT_MECHANICS speeds up lock picking gun safe
    p.moves -= std::max( 0, to_turns<int>( 10_minutes - time_duration::from_minutes( pick_quality ) )
                         - ( p.dex_cur + p.get_skill_level( skill_mechanics ) ) * 5 );

    ///\EFFECT_DEX increases chance of lock picking gun safe
    ///\EFFECT_MECHANICS increases chance of lock picking gun safe
    int pick_roll = ( dice( 2, p.get_skill_level( skill_mechanics ) ) + dice( 2,
                      p.dex_cur ) ) * pick_quality;
    int door_roll = dice( 4, 30 );
    if( pick_roll >= door_roll ) {
        p.practice( skill_mechanics, 1 );
        add_msg( _( "You successfully unlock the gun safe." ) );
        g->m.furn_set( examp, furn_str_id( "f_safe_o" ) );
    } else if( door_roll > ( 3 * pick_roll ) ) {
        add_msg( _( "Your clumsy attempt jams the lock!" ) );
        g->m.furn_set( examp, furn_str_id( "f_gunsafe_mj" ) );
    } else {
        add_msg( _( "The gun safe stumps your efforts to pick it." ) );
    }
}

/**
 * Attempt to "hack" the gunsafe's electronic lock and open it.
 */
void iexamine::gunsafe_el( player &p, const tripoint &examp )
{
    p.assign_activity( activity_id( "ACT_HACK_SAFE" ), to_moves<int>( 5_minutes ) );
    p.activity.placement = examp;
}

/**
 * Checks PC has a crowbar then calls iuse.crowbar.
 */
void iexamine::locked_object( player &p, const tripoint &examp )
{
    auto prying_items = p.crafting_inventory().items_with( []( const item & it ) -> bool {
        item temporary_item( it.type );
        return temporary_item.has_quality( quality_id( "PRY" ), 1 );
    } );

    if( prying_items.empty() ) {
        add_msg( m_info, _( "If only you had something to pry with..." ) );
        return;
    }

    // Sort by their quality level.
    std::sort( prying_items.begin(), prying_items.end(), []( const item * a, const item * b ) -> bool {
        return a->get_quality( quality_id( "PRY" ) ) > b->get_quality( quality_id( "PRY" ) );
    } );

    p.add_msg_if_player( _( "You attempt to pry open the %s using your %s..." ),
                         g->m.has_furn( examp ) ? g->m.furnname( examp ) : g->m.tername( examp ), prying_items[0]->tname() );

    // if crowbar() ever eats charges or otherwise alters the passed item, rewrite this to reflect
    // changes to the original item.
    iuse dummy;
    item temporary_item( prying_items[0]->type );
    dummy.crowbar( &p, &temporary_item, false, examp );
}

void iexamine::bulletin_board( player &p, const tripoint &examp )
{
    g->validate_camps();
    point omt = ms_to_omt_copy( g->m.getabs( examp.xy() ) );
    cata::optional<basecamp *> bcp = overmap_buffer.find_camp( omt );
    if( bcp ) {
        basecamp *temp_camp = *bcp;
        temp_camp->validate_assignees();
        temp_camp->validate_sort_points();

        const std::string title = ( "Base Missions" );
        mission_data mission_key;
        temp_camp->get_available_missions( mission_key, false );
        if( talk_function::display_and_choose_opts( mission_key, temp_camp->camp_omt_pos(),
                "FACTION_CAMP", title ) ) {
            temp_camp->handle_mission( mission_key.cur_key.id, mission_key.cur_key.dir, false );
        }
    } else {
        p.add_msg_if_player( _( "This bulletin board is not inside a camp" ) );
    }
}

/**
 * Display popup with reference to "The Enigma of Amigara Fault."
 */
void iexamine::fault( player &, const tripoint & )
{
    popup( _( "This wall is perfectly vertical.  Odd, twisted holes are set in it, leading\n"
              "as far back into the solid rock as you can see.  The holes are humanoid in\n"
              "shape, but with long, twisted, distended limbs." ) );
}

/**
 * Spawn 1d4 wyrms and sink pedestal into ground.
 */
void iexamine::pedestal_wyrm( player &p, const tripoint &examp )
{
    if( !g->m.i_at( examp ).empty() ) {
        none( p, examp );
        return;
    }
    // Send in a few wyrms to start things off.
    g->events().send<event_type::awakes_dark_wyrms>();
    int num_wyrms = rng( 1, 4 );
    for( int i = 0; i < num_wyrms; i++ ) {
        int tries = 0;
        tripoint monp = examp;
        do {
            monp.x = rng( 0, MAPSIZE_X );
            monp.y = rng( 0, MAPSIZE_Y );
            tries++;
        } while( tries < 10 && !g->is_empty( monp ) &&
                 rl_dist( p.pos(), monp ) <= 2 );
        if( tries < 10 ) {
            g->m.ter_set( monp, t_rock_floor );
            g->summon_mon( mon_dark_wyrm, monp );
        }
    }
    add_msg( _( "The pedestal sinks into the ground..." ) );
    sounds::sound( examp, 80, sounds::sound_t::combat, _( "an ominous grinding noise..." ), true,
                   "misc", "stones_grinding" );
    g->m.ter_set( examp, t_rock_floor );
    g->timed_events.add( TIMED_EVENT_SPAWN_WYRMS, calendar::turn + rng( 30_seconds, 60_seconds ) );
}

/**
 * Put petrified eye on pedestal causing it to sink into ground and open temple.
 */
void iexamine::pedestal_temple( player &p, const tripoint &examp )
{
    map_stack items = g->m.i_at( examp );
    if( !items.empty() && items.only_item().typeId() == "petrified_eye" ) {
        add_msg( _( "The pedestal sinks into the ground..." ) );
        g->m.ter_set( examp, t_dirt );
        g->m.i_clear( examp );
        g->timed_events.add( TIMED_EVENT_TEMPLE_OPEN, calendar::turn + 10_seconds );
    } else if( p.has_amount( "petrified_eye", 1 ) &&
               query_yn( _( "Place your petrified eye on the pedestal?" ) ) ) {
        p.use_amount( "petrified_eye", 1 );
        add_msg( _( "The pedestal sinks into the ground..." ) );
        g->m.ter_set( examp, t_dirt );
        g->timed_events.add( TIMED_EVENT_TEMPLE_OPEN, calendar::turn + 10_seconds );
    } else {
        add_msg( _( "This pedestal is engraved in eye-shaped diagrams, and has a "
                    "large semi-spherical indentation at the top." ) );
    }
}

/**
 * Unlock/open door or attempt to peek through peephole.
 */
void iexamine::door_peephole( player &p, const tripoint &examp )
{
    if( g->m.is_outside( p.pos() ) ) {
        // if door is a locked type attempt to open
        if( g->m.has_flag( "OPENCLOSE_INSIDE", examp ) ) {
            locked_object( p, examp );
        } else {
            p.add_msg_if_player( _( "You cannot look through the peephole from the outside." ) );
        }

        return;
    }

    // Peek through the peephole, or open the door.
    const int choice = uilist( _( "Do what with the door?" ), {
        _( "Peek through peephole." ),
        _( "Open door." )
    } );
    if( choice == 0 ) {
        // Peek
        g->peek( examp );
        p.add_msg_if_player( _( "You peek through the peephole." ) );
    } else if( choice == 1 ) {
        g->m.open_door( examp, true, false );
        p.add_msg_if_player( _( "You open the door." ) );
    } else {
        p.add_msg_if_player( _( "Never mind." ) );
    }
}

void iexamine::fswitch( player &p, const tripoint &examp )
{
    if( !query_yn( _( "Flip the %s?" ), g->m.tername( examp ) ) ) {
        none( p, examp );
        return;
    }
    ter_id terid = g->m.ter( examp );
    p.moves -= to_moves<int>( 1_seconds );
    tripoint tmp;
    tmp.z = examp.z;
    for( tmp.y = examp.y; tmp.y <= examp.y + 5; tmp.y++ ) {
        for( tmp.x = 0; tmp.x < MAPSIZE_X; tmp.x++ ) {
            if( terid == t_switch_rg ) {
                if( g->m.ter( tmp ) == t_rock_red ) {
                    g->m.ter_set( tmp, t_floor_red );
                } else if( g->m.ter( tmp ) == t_floor_red ) {
                    g->m.ter_set( tmp, t_rock_red );
                } else if( g->m.ter( tmp ) == t_rock_green ) {
                    g->m.ter_set( tmp, t_floor_green );
                } else if( g->m.ter( tmp ) == t_floor_green ) {
                    g->m.ter_set( tmp, t_rock_green );
                }
            } else if( terid == t_switch_gb ) {
                if( g->m.ter( tmp ) == t_rock_blue ) {
                    g->m.ter_set( tmp, t_floor_blue );
                } else if( g->m.ter( tmp ) == t_floor_blue ) {
                    g->m.ter_set( tmp, t_rock_blue );
                } else if( g->m.ter( tmp ) == t_rock_green ) {
                    g->m.ter_set( tmp, t_floor_green );
                } else if( g->m.ter( tmp ) == t_floor_green ) {
                    g->m.ter_set( tmp, t_rock_green );
                }
            } else if( terid == t_switch_rb ) {
                if( g->m.ter( tmp ) == t_rock_blue ) {
                    g->m.ter_set( tmp, t_floor_blue );
                } else if( g->m.ter( tmp ) == t_floor_blue ) {
                    g->m.ter_set( tmp, t_rock_blue );
                } else if( g->m.ter( tmp ) == t_rock_red ) {
                    g->m.ter_set( tmp, t_floor_red );
                } else if( g->m.ter( tmp ) == t_floor_red ) {
                    g->m.ter_set( tmp, t_rock_red );
                }
            } else if( terid == t_switch_even ) {
                if( ( tmp.y - examp.y ) % 2 == 1 ) {
                    if( g->m.ter( tmp ) == t_rock_red ) {
                        g->m.ter_set( tmp, t_floor_red );
                    } else if( g->m.ter( tmp ) == t_floor_red ) {
                        g->m.ter_set( tmp, t_rock_red );
                    } else if( g->m.ter( tmp ) == t_rock_green ) {
                        g->m.ter_set( tmp, t_floor_green );
                    } else if( g->m.ter( tmp ) == t_floor_green ) {
                        g->m.ter_set( tmp, t_rock_green );
                    } else if( g->m.ter( tmp ) == t_rock_blue ) {
                        g->m.ter_set( tmp, t_floor_blue );
                    } else if( g->m.ter( tmp ) == t_floor_blue ) {
                        g->m.ter_set( tmp, t_rock_blue );
                    }
                }
            }
        }
    }
    add_msg( m_warning, _( "You hear the rumble of rock shifting." ) );
    g->timed_events.add( TIMED_EVENT_TEMPLE_SPAWN, calendar::turn + 3_turns );
}

/**
 * If it's winter: show msg and return true. Otherwise return false
 */
static bool dead_plant( bool flower, player &p, const tripoint &examp )
{
    if( season_of_year( calendar::turn ) == WINTER ) {
        if( flower ) {
            add_msg( m_info, _( "This flower is dead. You can't get it." ) );
        } else {
            add_msg( m_info, _( "This plant is dead. You can't get it." ) );
        }

        iexamine::none( p, examp );
        return true;
    }

    return false;
}

/**
 * Helper method to see if player has traits, hunger and mouthwear for drinking nectar.
 */
static bool can_drink_nectar( const player &p )
{
    return ( p.has_active_mutation( trait_id( "PROBOSCIS" ) )  ||
             p.has_active_mutation( trait_id( "BEAK_HUM" ) ) ) &&
           ( ( p.get_hunger() ) > 0 ) && ( !( p.wearing_something_on( bp_mouth ) ) );
}

/**
 * Consume Nectar. -15 hunger.
 */
static bool drink_nectar( player &p )
{
    if( can_drink_nectar( p ) ) {
        p.moves -= to_moves<int>( 30_seconds );
        add_msg( _( "You drink some nectar." ) );
        item nectar( "nectar", calendar::turn, 1 );
        p.eat( nectar );
        return true;
    }

    return false;
}

/**
 * Spawn an item after harvesting the plant
 */
static void handle_harvest( player &p, const std::string &itemid, bool force_drop )
{
    item harvest = item( itemid );
    if( !force_drop && p.can_pickVolume( harvest, true ) &&
        p.can_pickWeight( harvest, true ) ) {
        p.i_add( harvest );
        p.add_msg_if_player( _( "You harvest: %s." ), harvest.tname() );
    } else {
        g->m.add_item_or_charges( p.pos(), harvest );
        p.add_msg_if_player( _( "You harvest and drop: %s." ), harvest.tname() );
    }
}

/**
 * Prompt pick (or drink nectar if able) poppy bud. Not safe for player.
 *
 * Drinking causes: -25 hunger, +20 fatigue, pkill2-70 effect and, 1 in 20 pkiller-1 addiction.
 * Picking w/ env_resist < 5 causes 1 in 3  sleep for 12 min and 4 dmg to each leg
 */
void iexamine::flower_poppy( player &p, const tripoint &examp )
{
    if( dead_plant( true, p, examp ) ) {
        return;
    }
    // TODO: Get rid of this section and move it to eating
    // Two y/n prompts is just too much
    if( can_drink_nectar( p ) ) {
        if( !query_yn( _( "You feel woozy as you explore the %s. Drink?" ),
                       g->m.furnname( examp ) ) ) {
            return;
        }
        p.moves -= to_moves<int>( 30_seconds ); // You take your time...
        add_msg( _( "You slowly suck up the nectar." ) );
        item poppy( "poppy_nectar", calendar::turn, 1 );
        p.eat( poppy );
        p.mod_fatigue( 20 );
        p.add_effect( effect_pkill2, 7_minutes );
        // Please drink poppy nectar responsibly.
        if( one_in( 20 ) ) {
            p.add_addiction( ADD_PKILLER, 1 );
        }
    }
    if( !query_yn( _( "Pick %s?" ), g->m.furnname( examp ) ) ) {
        none( p, examp );
        return;
    }

    int resist = p.get_env_resist( bp_mouth );

    if( resist < 10 ) {
        // Can't smell the flowers with a gas mask on!
        add_msg( m_warning, _( "This flower has a heady aroma." ) );
    }

    auto recentWeather = sum_conditions( calendar::turn - 10_minutes, calendar::turn, p.pos() );

    // If it has been raining recently, then this event is twice less likely.
    if( ( ( recentWeather.rain_amount > 1 ) ? one_in( 6 ) : one_in( 3 ) ) && resist < 5 ) {
        // Should user player::infect, but can't!
        // player::infect needs to be restructured to return a bool indicating success.
        add_msg( m_bad, _( "You fall asleep..." ) );
        p.fall_asleep( 2_hours );
        add_msg( m_bad, _( "Your legs are covered in the poppy's roots!" ) );
        p.apply_damage( nullptr, bp_leg_l, 4 );
        p.apply_damage( nullptr, bp_leg_r, 4 );
        p.moves -= 50;
    }

    g->m.furn_set( examp, f_null );

    handle_harvest( p, "poppy_bud", false );
    handle_harvest( p, "withered", false );
}

/**
 * Prompt pick cactus pad. Not safe for player.
 */
void iexamine::flower_cactus( player &p, const tripoint &examp )
{
    if( dead_plant( true, p, examp ) ) {
        return;
    }

    if( !query_yn( _( "Pick %s?" ), g->m.furnname( examp ) ) ) {
        none( p, examp );
        return;
    }

    if( one_in( 6 ) ) {
        add_msg( m_bad, _( "The cactus' nettles sting you!" ) );
        p.apply_damage( nullptr, bp_arm_l, 4 );
        p.apply_damage( nullptr, bp_arm_r, 4 );
    }

    g->m.furn_set( examp, f_null );

    handle_harvest( p, "cactus_pad", false );
    handle_harvest( p, "seed_cactus", false );
}

/**
 * Dig up its roots or drink its nectar if you can.
 */
void iexamine::flower_dahlia( player &p, const tripoint &examp )
{
    if( dead_plant( true, p, examp ) ) {
        return;
    }

    if( drink_nectar( p ) ) {
        return;
    }

    bool can_get_root = p.has_quality( quality_id( "DIG" ) ) || p.has_trait( trait_BURROW );

    if( can_get_root ) {
        if( !query_yn( _( "Pick %s?" ), g->m.furnname( examp ) ) ) {
            none( p, examp );
            return;
        }
    } else {
        if( !query_yn( _( "You don't have a digging tool to dig up roots. Pick %s anyway?" ),
                       g->m.furnname( examp ) ) ) {
            none( p, examp );
            return;
        }
    }

    g->m.furn_set( examp, f_null );

    if( can_get_root ) {
        handle_harvest( p, "dahlia_root", false );
    }
    handle_harvest( p, "seed_dahlia", false );
    handle_harvest( p, "withered", false );
    // There was a bud and flower spawn here
    // But those were useless, don't re-add until they get useful
}

static bool harvest_common( player &p, const tripoint &examp, bool furn, bool nectar,
                            bool auto_forage = false )
{
    const auto hid = g->m.get_harvest( examp );
    if( hid.is_null() || hid->empty() ) {
        if( !auto_forage ) {
            p.add_msg_if_player( m_info, _( "Nothing can be harvested from this plant in current season." ) );
        }
        if( p.manual_examine ) {
            iexamine::none( p, examp );
        }
        return false;
    }

    const auto &harvest = hid.obj();

    // If nothing can be harvested, neither can nectar
    // Incredibly low priority TODO: Allow separating nectar seasons
    if( nectar && drink_nectar( p ) ) {
        return false;
    }

    if( p.is_player() && !auto_forage &&
        !query_yn( _( "Pick %s?" ), furn ? g->m.furnname( examp ) : g->m.tername(
                       examp ) ) ) {
        iexamine::none( p, examp );
        return false;
    }

    int lev = p.get_skill_level( skill_survival );
    bool got_anything = false;
    for( const auto &entry : harvest ) {
        float min_num = entry.base_num.first + lev * entry.scale_num.first;
        float max_num = entry.base_num.second + lev * entry.scale_num.second;
        int roll = std::min<int>( entry.max, round( rng_float( min_num, max_num ) ) );
        if( roll >= 1 ) {
            got_anything = true;
            for( int i = 0; i < roll; i++ ) {
                handle_harvest( p, entry.drop, false );
            }
        }
    }

    if( !got_anything ) {
        p.add_msg_if_player( m_bad, _( "You couldn't harvest anything." ) );
    }

    iexamine::practice_survival_while_foraging( &p );

    p.mod_moves( -to_moves<int>( rng( 5_seconds, 15_seconds ) ) );
    return true;
}

void iexamine::harvest_furn_nectar( player &p, const tripoint &examp )
{
    bool auto_forage = get_option<bool>( "AUTO_FEATURES" ) &&
                       get_option<std::string>( "AUTO_FORAGING" ) == "both";
    if( harvest_common( p, examp, true, true, auto_forage ) ) {
        g->m.furn_set( examp, f_null );
    }
}

void iexamine::harvest_furn( player &p, const tripoint &examp )
{
    bool auto_forage = get_option<bool>( "AUTO_FEATURES" ) &&
                       get_option<std::string>( "AUTO_FORAGING" ) == "both";
    if( harvest_common( p, examp, true, false, auto_forage ) ) {
        g->m.furn_set( examp, f_null );
    }
}

void iexamine::harvest_ter_nectar( player &p, const tripoint &examp )
{
    bool auto_forage = get_option<bool>( "AUTO_FEATURES" ) &&
                       ( get_option<std::string>( "AUTO_FORAGING" ) == "both" ||
                         get_option<std::string>( "AUTO_FORAGING" ) == "bushes" ||
                         get_option<std::string>( "AUTO_FORAGING" ) == "trees" );
    if( harvest_common( p, examp, false, true, auto_forage ) ) {
        g->m.ter_set( examp, g->m.get_ter_transforms_into( examp ) );
    }
}

void iexamine::harvest_ter( player &p, const tripoint &examp )
{
    bool auto_forage = get_option<bool>( "AUTO_FEATURES" ) &&
                       ( get_option<std::string>( "AUTO_FORAGING" ) == "both" ||
                         get_option<std::string>( "AUTO_FORAGING" ) == "trees" );
    if( harvest_common( p, examp, false, false, auto_forage ) ) {
        g->m.ter_set( examp, g->m.get_ter_transforms_into( examp ) );
    }
}

/**
 * Only harvest a plant once per season.  Display message and call iexamine::none.
 */
void iexamine::harvested_plant( player &p, const tripoint &examp )
{
    p.add_msg_if_player( m_info, _( "Nothing can be harvested from this plant in current season" ) );
    iexamine::none( p, examp );
}

void iexamine::flower_marloss( player &p, const tripoint &examp )
{
    if( season_of_year( calendar::turn ) == WINTER ) {
        add_msg( m_info, _( "This flower is still alive, despite the harsh conditions..." ) );
    }
    if( can_drink_nectar( p ) ) {
        if( !query_yn( _( "You feel out of place as you explore the %s. Drink?" ),
                       g->m.furnname( examp ) ) ) {
            return;
        }
        p.moves -= to_moves<int>( 30_seconds ); // Takes 30 seconds
        add_msg( m_bad, _( "This flower tastes very wrong..." ) );
        // If you can drink flowers, you're post-thresh and the Mycus does not want you.
        p.add_effect( effect_teleglow, 10_minutes );
    }
    if( !query_yn( _( "Pick %s?" ), g->m.furnname( examp ) ) ) {
        none( p, examp );
        return;
    }
    g->m.furn_set( examp, f_null );
    g->m.spawn_item( p.pos(), "marloss_seed", 1, 3, calendar::turn );
    handle_harvest( p, "withered", false );
}

/**
 * Spawn spiders from a spider egg sack in radius 1 around the egg sack.
 * Transforms the egg sack furniture into a ruptured egg sack (f_egg_sacke).
 * Also spawns eggs.
 * @param p The player
 * @param examp Location of egg sack
 * @param montype The monster type of the created spiders.
 */
void iexamine::egg_sack_generic( player &p, const tripoint &examp,
                                 const mtype_id &montype )
{
    const std::string old_furn_name = g->m.furnname( examp );
    if( !query_yn( _( "Harvest the %s?" ), old_furn_name ) ) {
        none( p, examp );
        return;
    }
    g->m.furn_set( examp, f_egg_sacke );
    int monster_count = 0;
    if( one_in( 2 ) ) {
        const std::vector<tripoint> pts = closest_tripoints_first( 1, examp );
        for( const auto &pt : pts ) {
            if( g->is_empty( pt ) && one_in( 3 ) ) {
                g->summon_mon( montype, pt );
                monster_count++;
            }
        }
    }
    int roll = rng( 1, 5 );
    bool drop_eggs = monster_count >= 1;
    for( int i = 0; i < roll; i++ ) {
        handle_harvest( p, "spider_egg", drop_eggs );
    }
    if( monster_count == 1 ) {
        add_msg( m_warning, _( "A spiderling bursts from the %s!" ), old_furn_name );
    } else if( monster_count >= 1 ) {
        add_msg( m_warning, _( "Spiderlings burst from the %s!" ), old_furn_name );
    }
}

void iexamine::egg_sackbw( player &p, const tripoint &examp )
{
    egg_sack_generic( p, examp, mon_spider_widow_giant_s );
}

void iexamine::egg_sackcs( player &p, const tripoint &examp )
{
    egg_sack_generic( p, examp, mon_spider_cellar_giant_s );
}

void iexamine::egg_sackws( player &p, const tripoint &examp )
{
    egg_sack_generic( p, examp, mon_spider_web_s );
}

/**
 * Remove furniture. Add spore effect.
 */
void iexamine::fungus( player &p, const tripoint &examp )
{
    add_msg( _( "The %s crumbles into spores!" ), g->m.furnname( examp ) );
    fungal_effects( *g, g->m ).create_spores( examp, &p );
    g->m.furn_set( examp, f_null );
    p.moves -= 50;
}

/**
 *  Make lists of unique seed types and names for the menu(no multiple hemp seeds etc)
 */
std::vector<seed_tuple> iexamine::get_seed_entries( const std::vector<item *> &seed_inv )
{
    std::map<itype_id, int> seed_map;
    for( auto &seed : seed_inv ) {
        seed_map[seed->typeId()] += ( seed->charges > 0 ? seed->charges : 1 );
    }

    std::vector<seed_tuple> seed_entries;
    seed_entries.reserve( seed_map.size() );
    for( const auto &pr : seed_map ) {
        seed_entries.emplace_back(
            pr.first, item::nname( pr.first, pr.second ), pr.second );
    }

    // Sort by name
    std::sort( seed_entries.begin(), seed_entries.end(),
    []( const seed_tuple & l, const seed_tuple & r ) {
        return std::get<1>( l ).compare( std::get<1>( r ) ) < 0;
    } );

    return seed_entries;
}

/**
 *  Choose seed for planting
 */
int iexamine::query_seed( const std::vector<seed_tuple> &seed_entries )
{
    uilist smenu;

    smenu.text = _( "Use which seed?" );
    int count = 0;
    for( const auto &entry : seed_entries ) {
        const std::string &seed_name = std::get<1>( entry );
        int seed_count = std::get<2>( entry );

        std::string format = seed_count > 0 ? "%s (%d)" : "%s";

        smenu.addentry( count++, true, MENU_AUTOASSIGN, format.c_str(),
                        seed_name, seed_count );
    }

    smenu.query();

    if( smenu.ret >= 0 ) {
        return smenu.ret;
    } else {
        return seed_entries.size();
    }
}

/**
 *  Actual planting of selected seed
 */
void iexamine::plant_seed( player &p, const tripoint &examp, const itype_id &seed_id )
{
    std::list<item> used_seed;
    if( item::count_by_charges( seed_id ) ) {
        used_seed = p.use_charges( seed_id, 1 );
    } else {
        used_seed = p.use_amount( seed_id, 1 );
    }
    if( !used_seed.empty() ) {
        used_seed.front().set_age( 0_turns );
        if( used_seed.front().has_var( "activity_var" ) ) {
            used_seed.front().erase_var( "activity_var" );
        }
        g->m.add_item_or_charges( examp, used_seed.front() );
        if( g->m.has_flag_furn( "PLANTABLE", examp ) ) {
            g->m.furn_set( examp, furn_str_id( g->m.furn( examp )->plant->transform ) );
        } else {
            g->m.set( examp, t_dirt, f_plant_seed );
        }
        p.moves -= to_moves<int>( 30_seconds );
        p.add_msg_player_or_npc( _( "You plant some %s." ), _( "<npcname> plants some %s." ),
                                 item::nname( seed_id ) );
    }
}

/**
 * If it's warm enough, pick one of the player's seeds and plant it.
 */
void iexamine::dirtmound( player &p, const tripoint &examp )
{

    if( !warm_enough_to_plant( g->u.pos() ) ) {
        add_msg( m_info, _( "It is too cold to plant anything now." ) );
        return;
    }
    /* ambient_light_at() not working?
    if (g->m.ambient_light_at(examp) < LIGHT_AMBIENT_LOW) {
        add_msg(m_info, _("It is too dark to plant anything now."));
        return;
    }*/
    std::vector<item *> seed_inv = p.items_with( []( const item & itm ) {
        return itm.is_seed();
    } );
    if( seed_inv.empty() ) {
        add_msg( m_info, _( "You have no seeds to plant." ) );
        return;
    }
    if( !g->m.i_at( examp ).empty() ) {
        add_msg( _( "Something's lying there..." ) );
        return;
    }

    auto seed_entries = get_seed_entries( seed_inv );

    int seed_index = query_seed( seed_entries );

    // Did we cancel?
    if( seed_index < 0 || seed_index >= static_cast<int>( seed_entries.size() ) ) {
        add_msg( _( "You saved your seeds for later." ) );
        return;
    }
    const auto &seed_id = std::get<0>( seed_entries[seed_index] );

    plant_seed( p, examp, seed_id );
}

/**
 * Items that appear when a generic plant is harvested. Seed @ref islot_seed.
 * @param type The seed type, must have a @ref itype::seed slot.
 * @param plant_count Number of fruits to generate. For charge-based items, this
 *     specifies multiples of the default charge.
 * @param seed_count Number of seeds to generate.
 * @param byproducts If true, byproducts (like straw, withered plants, see
 * @ref islot_seed::byproducts) are included.
 */
std::list<item> iexamine::get_harvest_items( const itype &type, const int plant_count,
        const int seed_count, const bool byproducts )
{
    std::list<item> result;
    if( !type.seed ) {
        return result;
    }
    const islot_seed &seed_data = *type.seed;
    // This is a temporary measure, itype should instead provide appropriate accessors
    // to expose data about the seed item to allow harvesting to function.
    const itype_id &seed_type = type.get_id();

    const auto add = [&]( const itype_id & id, const int count ) {
        item new_item( id, calendar::turn );
        if( new_item.count_by_charges() && count > 0 ) {
            new_item.charges *= count;
            new_item.charges /= seed_data.fruit_div;
            if( new_item.charges <= 0 ) {
                new_item.charges = 1;
            }
            result.push_back( new_item );
        } else if( count > 0 ) {
            result.insert( result.begin(), count, new_item );
        }
    };

    if( seed_data.spawn_seeds ) {
        add( seed_type, seed_count );
    }

    add( seed_data.fruit_id, plant_count );

    if( byproducts ) {
        for( auto &b : seed_data.byproducts ) {
            add( b, 1 );
        }
    }

    return result;
}

/**
 * Actual harvesting of selected plant
 */
void iexamine::harvest_plant( player &p, const tripoint &examp, bool from_activity )
{
    // Can't use item_stack::only_item() since there might be fertilizer
    map_stack items = g->m.i_at( examp );
    const map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
        return it.is_seed();
    } );

    if( seed == items.end() ) {
        debugmsg( "Missing seed for plant at (%d, %d, %d)", examp.x, examp.y, examp.z );
        g->m.i_clear( examp );
        g->m.furn_set( examp, f_null );
        return;
    }

    const std::string &seedType = seed->typeId();
    if( seedType == "fungal_seeds" ) {
        fungus( p, examp );
        g->m.i_clear( examp );
    } else if( seedType == "marloss_seed" ) {
        fungus( p, examp );
        g->m.i_clear( examp );
        if( p.has_trait( trait_M_DEPENDENT ) && ( p.get_kcal_percent() < 0.8f || p.get_thirst() > 300 ) ) {
            g->m.ter_set( examp, t_marloss );
            add_msg( m_info,
                     _( "We have altered this unit's configuration to extract and provide local nutriment.  The Mycus provides." ) );
        } else if( ( p.has_trait( trait_M_DEFENDER ) ) || ( ( p.has_trait( trait_M_SPORES ) ||
                   p.has_trait( trait_M_FERTILE ) ) &&
                   one_in( 2 ) ) ) {
            g->summon_mon( mon_fungal_blossom, examp );
            add_msg( m_info, _( "The seed blooms forth!  We have brought true beauty to this world." ) );
        } else if( ( p.has_trait( trait_THRESH_MYCUS ) ) || one_in( 4 ) ) {
            g->m.furn_set( examp, f_flower_marloss );
            add_msg( m_info, _( "The seed blossoms rather rapidly..." ) );
        } else {
            g->m.furn_set( examp, f_flower_fungal );
            add_msg( m_info, _( "The seed blossoms into a flower-looking fungus." ) );
        }
    } else { // Generic seed, use the seed item data
        const itype &type = *seed->type;
        g->m.i_clear( examp );

        int skillLevel = p.get_skill_level( skill_survival );
        ///\EFFECT_SURVIVAL increases number of plants harvested from a seed
        int plant_count = rng( skillLevel / 2, skillLevel );
        plant_count *= g->m.furn( examp )->plant->harvest_multiplier;
        const int max_harvest_count = get_option<int>( "MAX_HARVEST_COUNT" );
        if( plant_count >= max_harvest_count ) {
            plant_count = max_harvest_count;
        } else if( plant_count <= 0 ) {
            plant_count = 1;
        }
        const int seedCount = std::max( 1, rng( plant_count / 4, plant_count / 2 ) );
        for( auto &i : get_harvest_items( type, plant_count, seedCount, true ) ) {
            if( from_activity ) {
                i.set_var( "activity_var", p.name );
            }
            g->m.add_item_or_charges( examp, i );
        }
        g->m.furn_set( examp, furn_str_id( g->m.furn( examp )->plant->transform ) );
        p.moves -= to_moves<int>( 10_seconds );
    }
}

ret_val<bool> iexamine::can_fertilize( player &p, const tripoint &tile,
                                       const itype_id &fertilizer )
{
    if( !g->m.has_flag_furn( "PLANT", tile ) ) {
        return ret_val<bool>::make_failure( _( "Tile isn't a plant" ) );
    }
    if( g->m.i_at( tile ).size() > 1 ) {
        return ret_val<bool>::make_failure( _( "Tile is already fertilized" ) );
    }
    if( !p.has_charges( fertilizer, 1 ) ) {
        return ret_val<bool>::make_failure(
                   _( "Tried to fertilize with %s, but player doesn't have any." ),
                   fertilizer.c_str() );
    }

    return ret_val<bool>::make_success();
}

void iexamine::fertilize_plant( player &p, const tripoint &tile, const itype_id &fertilizer )
{
    ret_val<bool> can_fert = can_fertilize( p, tile, fertilizer );
    if( !can_fert.success() ) {
        debugmsg( can_fert.str() );
        return;
    }

    std::list<item> planted = p.use_charges( fertilizer, 1 );

    // Reduce the amount of time it takes until the next stage of the plant by
    // 20% of a seasons length. (default 2.8 days).
    const time_duration fertilizerEpoch = calendar::season_length() * 0.2;

    // Can't use item_stack::only_item() since there might be fertilizer
    map_stack items = g->m.i_at( tile );
    const map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
        return it.is_seed();
    } );
    if( seed == items.end() ) {
        debugmsg( "Missing seed for plant at (%d, %d, %d)", tile.x, tile.y, tile.z );
        g->m.i_clear( tile );
        g->m.furn_set( tile, f_null );
        return;
    }

    // TODO: item should probably clamp the value on its own
    seed->set_birthday( seed->birthday() - fertilizerEpoch );
    // The plant furniture has the NOITEM token which prevents adding items on that square,
    // spawned items are moved to an adjacent field instead, but the fertilizer token
    // must be on the square of the plant, therefore this hack:
    const auto old_furn = g->m.furn( tile );
    g->m.furn_set( tile, f_null );
    g->m.spawn_item( tile, "fertilizer", 1, 1, calendar::turn );
    g->m.furn_set( tile, old_furn );
    p.mod_moves( -to_moves<int>( 10_seconds ) );

    add_msg( m_info, _( "You fertilize the %s with the %s." ), seed->get_plant_name(),
             planted.front().tname() );
}

itype_id iexamine::choose_fertilizer( player &p, const std::string &pname, bool ask_player )
{
    std::vector<const item *> f_inv = p.all_items_with_flag( "FERTILIZER" );
    if( f_inv.empty() ) {
        add_msg( m_info, _( "You have no fertilizer for the %s." ), pname );
        return itype_id();
    }

    std::vector<itype_id> f_types;
    std::vector<std::string> f_names;
    for( auto &f : f_inv ) {
        if( std::find( f_types.begin(), f_types.end(), f->typeId() ) == f_types.end() ) {
            f_types.push_back( f->typeId() );
            f_names.push_back( f->tname() );
        }
    }

    if( ask_player && !query_yn( _( "Fertilize the %s" ), pname ) ) {
        return itype_id();
    }

    // Choose fertilizer from list
    int f_index = 0;
    if( f_types.size() > 1 ) {
        f_index = uilist( _( "Use which fertilizer?" ), f_names );
    }
    if( f_index < 0 ) {
        return itype_id();
    }

    return f_types[f_index];

}
void iexamine::aggie_plant( player &p, const tripoint &examp )
{
    // Can't use item_stack::only_item() since there might be fertilizer
    map_stack items = g->m.i_at( examp );
    const map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
        return it.is_seed();
    } );

    if( seed == items.end() ) {
        debugmsg( "Missing seed for plant at (%d, %d, %d)", examp.x, examp.y, examp.z );
        g->m.i_clear( examp );
        g->m.furn_set( examp, f_null );
        return;
    }

    const std::string pname = seed->get_plant_name();

    if( g->m.has_flag_furn( "GROWTH_HARVEST", examp ) && query_yn( _( "Harvest the %s?" ), pname ) ) {
        harvest_plant( p, examp );
    } else if( !g->m.has_flag_furn( "GROWTH_HARVEST", examp ) ) {
        if( g->m.i_at( examp ).size() > 1 ) {
            add_msg( m_info, _( "This %s has already been fertilized." ), pname );
            return;
        }
        itype_id fertilizer = choose_fertilizer( p, pname, true /*ask player for confirmation */ );

        if( !fertilizer.empty() ) {
            fertilize_plant( p, examp, fertilizer );
        }
    }
}

// Highly modified fermenting vat functions
void iexamine::kiln_empty( player &p, const tripoint &examp )
{
    furn_id cur_kiln_type = g->m.furn( examp );
    furn_id next_kiln_type = f_null;
    if( cur_kiln_type == f_kiln_empty ) {
        next_kiln_type = f_kiln_full;
    } else if( cur_kiln_type == f_kiln_metal_empty ) {
        next_kiln_type = f_kiln_metal_full;
    } else {
        debugmsg( "Examined furniture has action kiln_empty, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }

    static const std::set<material_id> kilnable{ material_id( "wood" ), material_id( "bone" ) };
    bool fuel_present = false;
    auto items = g->m.i_at( examp );
    for( const item &i : items ) {
        if( i.typeId() == "charcoal" ) {
            add_msg( _( "This kiln already contains charcoal." ) );
            add_msg( _( "Remove it before firing the kiln again." ) );
            return;
        } else if( i.made_of_any( kilnable ) ) {
            fuel_present = true;
        } else {
            add_msg( m_bad, _( "This kiln contains %s, which can't be made into charcoal!" ), i.tname( 1,
                     false ) );
            return;
        }
    }

    if( !fuel_present ) {
        add_msg( _( "This kiln is empty. Fill it with wood or bone and try again." ) );
        return;
    }

    ///\EFFECT_FABRICATION decreases loss when firing a kiln
    const int skill = p.get_skill_level( skill_fabrication );
    int loss = 60 - 2 *
               skill; // We can afford to be inefficient - logs and skeletons are cheap, charcoal isn't

    // Burn stuff that should get charred, leave out the rest
    units::volume total_volume = 0_ml;
    for( const item &i : items ) {
        total_volume += i.volume();
    }

    auto char_type = item::find_type( "unfinished_charcoal" );
    int char_charges = char_type->charges_per_volume( ( 100 - loss ) * total_volume / 100 );
    if( char_charges < 1 ) {
        add_msg( _( "The batch in this kiln is too small to yield any charcoal." ) );
        return;
    }

    if( !p.has_charges( "fire", 1 ) ) {
        add_msg( _( "This kiln is ready to be fired, but you have no fire source." ) );
        return;
    } else {
        add_msg( _( "This kiln contains %s %s of material, and is ready to be fired." ),
                 format_volume( total_volume ), volume_units_abbr() );
        if( !query_yn( _( "Fire the kiln?" ) ) ) {
            return;
        }
    }

    p.use_charges( "fire", 1 );
    g->m.i_clear( examp );
    g->m.furn_set( examp, next_kiln_type );
    item result( "unfinished_charcoal", calendar::turn );
    result.charges = char_charges;
    g->m.add_item( examp, result );
    add_msg( _( "You fire the charcoal kiln." ) );
}

void iexamine::kiln_full( player &, const tripoint &examp )
{
    furn_id cur_kiln_type = g->m.furn( examp );
    furn_id next_kiln_type = f_null;
    if( cur_kiln_type == f_kiln_full ) {
        next_kiln_type = f_kiln_empty;
    } else if( cur_kiln_type == f_kiln_metal_full ) {
        next_kiln_type = f_kiln_metal_empty;
    } else {
        debugmsg( "Examined furniture has action kiln_full, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }

    map_stack items = g->m.i_at( examp );
    if( items.empty() ) {
        add_msg( _( "This kiln is empty..." ) );
        g->m.furn_set( examp, next_kiln_type );
        return;
    }
    auto char_type = item::find_type( "charcoal" );
    add_msg( _( "There's a charcoal kiln there." ) );
    const time_duration firing_time = 6_hours; // 5 days in real life
    const time_duration time_left = firing_time - items.only_item().age();
    if( time_left > 0_turns ) {
        int hours = to_hours<int>( time_left );
        int minutes = to_minutes<int>( time_left ) + 1;
        if( minutes > 60 ) {
            add_msg( ngettext( "It will finish burning in about %d hour.",
                               "It will finish burning in about %d hours.",
                               hours ), hours );
        } else if( minutes > 30 ) {
            add_msg( _( "It will finish burning in less than an hour." ) );
        } else {
            add_msg( _( "It should take about %d minutes to finish burning." ), minutes );
        }
        return;
    }

    units::volume total_volume = 0_ml;
    // Burn stuff that should get charred, leave out the rest
    for( auto item_it = items.begin(); item_it != items.end(); ) {
        if( item_it->typeId() == "unfinished_charcoal" || item_it->typeId() == "charcoal" ) {
            total_volume += item_it->volume();
            item_it = items.erase( item_it );
        } else {
            item_it++;
        }
    }

    item result( "charcoal", calendar::turn );
    result.charges = char_type->charges_per_volume( total_volume );
    g->m.add_item( examp, result );
    g->m.furn_set( examp, next_kiln_type );
    add_msg( _( "It has finished burning, yielding %d charcoal." ), result.charges );
}
//arc furnance start
void iexamine::arcfurnace_empty( player &p, const tripoint &examp )
{
    furn_id cur_arcfurnace_type = g->m.furn( examp );
    furn_id next_arcfurnace_type = f_null;
    if( cur_arcfurnace_type == f_arcfurnace_empty ) {
        next_arcfurnace_type = f_arcfurnace_full;
    } else {
        debugmsg( "Examined furniture has action arcfurnace_empty, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }

    static const std::set<material_id> arcfurnaceable{ material_id( "cac2powder" ) };
    bool fuel_present = false;
    auto items = g->m.i_at( examp );
    for( const item &i : items ) {
        if( i.typeId() == "chem_carbide" ) {
            add_msg( _( "This furnace already contains calcium carbide." ) );
            add_msg( _( "Remove it before activating the arc furnace again." ) );
            return;
        } else if( i.made_of_any( arcfurnaceable ) ) {
            fuel_present = true;
        } else {
            add_msg( m_bad, _( "This furnace contains %s, which can't be made into calcium carbide!" ),
                     i.tname( 1, false ) );
            return;
        }
    }

    if( !fuel_present ) {
        add_msg( _( "This furance is empty. Fill it with powdered coke and lime mix, and try again." ) );
        return;
    }

    ///\EFFECT_FABRICATION decreases loss when firing a furnace
    const int skill = p.get_skill_level( skill_fabrication );
    int loss = 60 - 2 *
               skill; // Inefficency is still fine, coal and limestone is abundant

    // Burn stuff that should get charred, leave out the rest
    units::volume total_volume = 0_ml;
    for( const item &i : items ) {
        total_volume += i.volume();
    }

    auto char_type = item::find_type( "unfinished_cac2" );
    int char_charges = char_type->charges_per_volume( ( 100 - loss ) * total_volume / 100 );
    if( char_charges < 1 ) {
        add_msg( _( "The batch in this furance is too small to yield usable calcium carbide." ) );
        return;
    }
    //arc furnaces require a huge amount of current, so 1 full storage battery would work as a stand in
    if( !p.has_charges( "UPS", 1250 ) ) {
        add_msg( _( "This furnace is ready to be turned on, but you lack a UPS with sufficient power." ) );
        return;
    } else {
        add_msg( _( "This furnace contains %s %s of material, and is ready to be turned on." ),
                 format_volume( total_volume ), volume_units_abbr() );
        if( !query_yn( _( "Turn on the furnace?" ) ) ) {
            return;
        }
    }

    p.use_charges( "UPS", 1250 );
    g->m.i_clear( examp );
    g->m.furn_set( examp, next_arcfurnace_type );
    item result( "unfinished_cac2", calendar::turn );
    result.charges = char_charges;
    g->m.add_item( examp, result );
    add_msg( _( "You turn on the furnace." ) );
}

void iexamine::arcfurnace_full( player &, const tripoint &examp )
{
    furn_id cur_arcfurnace_type = g->m.furn( examp );
    furn_id next_arcfurnace_type = f_null;
    if( cur_arcfurnace_type == f_arcfurnace_full ) {
        next_arcfurnace_type = f_arcfurnace_empty;
    } else {
        debugmsg( "Examined furniture has action arcfurnace_full, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }

    map_stack items = g->m.i_at( examp );
    if( items.empty() ) {
        add_msg( _( "This furnace is empty..." ) );
        g->m.furn_set( examp, next_arcfurnace_type );
        return;
    }
    auto char_type = item::find_type( "chem_carbide" );
    add_msg( _( "There's an arc furnace there." ) );
    const time_duration firing_time = 2_hours; // Arc furnaces work really fast in reality
    const time_duration time_left = firing_time - items.only_item().age();
    if( time_left > 0_turns ) {
        int hours = to_hours<int>( time_left );
        int minutes = to_minutes<int>( time_left ) + 1;
        if( minutes > 60 ) {
            add_msg( ngettext( "It will finish burning in about %d hour.",
                               "It will finish burning in about %d hours.",
                               hours ), hours );
        } else if( minutes > 30 ) {
            add_msg( _( "It will finish burning in less than an hour." ) );
        } else {
            add_msg( _( "It should take about %d minutes to finish burning." ), minutes );
        }
        return;
    }

    units::volume total_volume = 0_ml;
    // Burn stuff that should get charred, leave out the rest
    for( auto item_it = items.begin(); item_it != items.end(); ) {
        if( item_it->typeId() == "unfinished_cac2" || item_it->typeId() == "chem_carbide" ) {
            total_volume += item_it->volume();
            item_it = items.erase( item_it );
        } else {
            item_it++;
        }
    }

    item result( "chem_carbide", calendar::turn );
    result.charges = char_type->charges_per_volume( total_volume );
    g->m.add_item( examp, result );
    g->m.furn_set( examp, next_arcfurnace_type );
    add_msg( _( "It has finished burning, yielding %d calcium carbide." ), result.charges );
}
//arc furnace end

void iexamine::fireplace( player &p, const tripoint &examp );

void iexamine::autoclave_empty( player &p, const tripoint &examp )
{
    furn_id cur_autoclave_type = g->m.furn( examp );
    furn_id next_autoclave_type = f_null;
    if( cur_autoclave_type == furn_id( "f_autoclave" ) ) {
        next_autoclave_type = furn_id( "f_autoclave_full" );
    } else {
        debugmsg( "Examined furniture has action autoclave_empty, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }

    map_stack items = g->m.i_at( examp );
    bool cbms = std::all_of( items.begin(), items.end(), []( const item & i ) {
        return i.is_bionic();
    } );

    bool filthy_cbms = std::all_of( items.begin(), items.end(), []( const item & i ) {
        return i.is_bionic() && i.has_flag( "FILTHY" );
    } );

    if( items.empty() ) {
        add_msg( _( "This autoclave is empty..." ) );
        return;
    }
    if( !cbms ) {
        add_msg( m_bad,
                 _( "You need to remove all non-CBM items from the autoclave to start the program." ) );
        return;
    }
    if( filthy_cbms ) {
        add_msg( m_bad,
                 _( "Some of those CBMs are filthy, you should wash them first for the sterilization process to work properly." ) );
        return;
    }
    auto reqs = *requirement_id( "autoclave" );

    if( !reqs.can_make_with_inventory( p.crafting_inventory(), is_crafting_component ) ) {
        popup( "%s", reqs.list_missing() );
        return;
    }

    if( query_yn( _( "Start the autoclave?" ) ) ) {

        for( const auto &e : reqs.get_components() ) {
            p.consume_items( e, 1, is_crafting_component );
        }
        for( const auto &e : reqs.get_tools() ) {
            p.consume_tools( e );
        }
        p.invalidate_crafting_inventory();
        for( item &it : items ) {
            it.set_birthday( calendar::turn );
        }
        g->m.furn_set( examp, next_autoclave_type );
        add_msg( _( "You start the autoclave." ) );
    }
}

void iexamine::autoclave_full( player &, const tripoint &examp )
{
    furn_id cur_autoclave_type = g->m.furn( examp );
    furn_id next_autoclave_type = f_null;
    if( cur_autoclave_type == furn_id( "f_autoclave_full" ) ) {
        next_autoclave_type = furn_id( "f_autoclave" );
    } else {
        debugmsg( "Examined furniture has action autoclave_full, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }

    map_stack items = g->m.i_at( examp );
    bool cbms = std::all_of( items.begin(), items.end(), []( const item & i ) {
        return i.is_bionic();
    } );

    bool cbms_not_packed = std::all_of( items.begin(), items.end(), []( const item & i ) {
        return i.is_bionic() && i.has_flag( "NO_PACKED" );
    } );

    if( items.empty() ) {
        add_msg( _( "This autoclave is empty..." ) );
        g->m.furn_set( examp, next_autoclave_type );
        return;
    }
    if( !cbms ) {
        add_msg( m_bad,
                 _( "ERROR Autoclave can't process non CBM items." ) );
        return;
    }
    add_msg( _( "The autoclave is running." ) );

    const item &clock = *items.begin();
    const time_duration Cycle_time = 90_minutes;
    const time_duration time_left = Cycle_time - clock.age();

    if( time_left > 0_turns ) {
        add_msg( _( "The cycle will be complete in %s." ), to_string( time_left ) );
        return;
    }

    g->m.furn_set( examp, next_autoclave_type );
    for( item &it : items ) {
        if( !it.has_flag( "NO_PACKED" ) ) {
            it.unset_flag( "NO_STERILE" );
        }
    }
    add_msg( m_good, _( "The cycle is complete, the CBMs are now sterile." ) );

    if( cbms_not_packed ) {
        add_msg( m_info,
                 _( "CBMs in direct contact with the environment will almost immediately become contaminated." ) );
    }
    g->m.furn_set( examp, next_autoclave_type );
}

void iexamine::fireplace( player &p, const tripoint &examp )
{
    const bool already_on_fire = g->m.has_nearby_fire( examp, 0 );
    const bool furn_is_deployed = !g->m.furn( examp ).obj().deployed_item.empty();

    std::multimap<int, item *> firestarters;
    for( item *it : p.items_with( []( const item & it ) {
    return it.has_flag( "FIRESTARTER" ) || it.has_flag( "FIRE" );
    } ) ) {
        const auto usef = it->type->get_use( "firestarter" );
        if( usef != nullptr && usef->get_actor_ptr() != nullptr ) {
            const auto actor = dynamic_cast<const firestarter_actor *>( usef->get_actor_ptr() );
            if( actor->can_use( p, *it, false, examp ).success() ) {
                firestarters.insert( std::pair<int, item *>( actor->moves_cost_fast, it ) );
            }
        }
    }

    const bool has_firestarter = !firestarters.empty();
    const bool has_bionic_firestarter = p.has_bionic( bionic_id( "bio_lighter" ) ) &&
                                        p.power_level >= bionic_id( "bio_lighter" )->power_activate;

    auto firequenchers = p.items_with( []( const item & it ) {
        return it.damage_melee( DT_BASH );
    } );

    uilist selection_menu;
    selection_menu.text = _( "Select an action" );
    selection_menu.addentry( 0, true, 'g', _( "Get items" ) );
    if( !already_on_fire ) {
        selection_menu.addentry( 1, has_firestarter, 'f',
                                 has_firestarter ? _( "Start a fire" ) : _( "Start a fire... you'll need a fire source." ) );
        if( has_bionic_firestarter ) {
            selection_menu.addentry( 2, true, 'b', _( "Use a CBM to start a fire" ) );
        }
    } else if( !firequenchers.empty() ) {
        selection_menu.addentry( 4, true, 'e', _( "Extinguish fire" ) );
    } else {
        selection_menu.addentry( 4, false, 'e', _( "Extinguish fire (bashing item required)" ) );
    }
    if( furn_is_deployed ) {
        selection_menu.addentry( 3, true, 't', _( "Take down the %s" ), g->m.furnname( examp ) );
    }
    selection_menu.query();

    switch( selection_menu.ret ) {
        case 0:
            none( p, examp );
            Pickup::pick_up( examp, 0 );
            return;
        case 1: {
            for( auto &firestarter : firestarters ) {
                item *it = firestarter.second;
                const auto usef = it->type->get_use( "firestarter" );
                const auto actor = dynamic_cast<const firestarter_actor *>( usef->get_actor_ptr() );
                p.add_msg_if_player( _( "You attempt to start a fire with your %s..." ), it->tname() );
                const ret_val<bool> can_use = actor->can_use( p, *it, false, examp );
                if( can_use.success() ) {
                    const int charges = actor->use( p, *it, false, examp );
                    p.use_charges( it->typeId(), charges );
                    return;
                } else {
                    p.add_msg_if_player( m_bad, can_use.str() );
                }
            }
            p.add_msg_if_player( _( "You weren't able to start a fire." ) );
            return;
        }
        case 2: {
            if( g->m.add_field( examp, fd_fire, 1 ) ) {
                p.charge_power( -bionic_id( "bio_lighter" )->power_activate );
                p.mod_moves( -to_moves<int>( 1_seconds ) );
            } else {
                p.add_msg_if_player( m_info, _( "You can't light a fire there." ) );
            }
            return;
        }
        case 3: {
            if( already_on_fire ) {
                if( !query_yn( _( "Really take down the %s while it's on fire?" ), g->m.furnname( examp ) ) ) {
                    return;
                }
            }
            p.add_msg_if_player( m_info, _( "You take down the %s." ),
                                 g->m.furnname( examp ) );
            const auto furn_item = g->m.furn( examp ).obj().deployed_item;
            g->m.add_item_or_charges( examp, item( furn_item, calendar::turn ) );
            g->m.furn_set( examp, f_null );
            return;
        }
        case 4: {
            g->m.remove_field( examp, fd_fire );
            p.mod_moves( -200 );
            p.add_msg_if_player( m_info, _( "With a few determined moves you put out the fire in the %s." ),
                                 g->m.furnname( examp ) );
            return;
        }
        default:
            none( p, examp );
            return;
    }
}

void iexamine::fvat_empty( player &p, const tripoint &examp )
{
    itype_id brew_type;
    std::string brew_nname;
    bool to_deposit = false;
    static const auto vat_volume = units::from_liter( 50 );
    bool vat_full = false;
    bool ferment = false;
    bool brew_present = false;
    int charges_on_ground = 0;
    auto items = g->m.i_at( examp );
    for( auto item_it = items.begin(); item_it != items.end(); ) {
        if( !item_it->is_brewable() || brew_present ) {
            // This isn't a brew or there was already another kind of brew inside,
            // so this has to be moved.
            items.insert( *item_it );
            // This will add items to a space near the vat, because it's flagged as NOITEM.
            item_it = items.erase( item_it );
        } else {
            item_it++;
            brew_present = true;
        }
    }
    if( !brew_present ) {
        add_msg( _( "This keg is empty." ) );
        // TODO: Allow using brews from crafting inventory
        const auto b_inv = p.items_with( []( const item & it ) {
            return it.is_brewable();
        } );
        if( b_inv.empty() ) {
            add_msg( m_info, _( "You have no brew to ferment." ) );
            return;
        }
        // Make lists of unique typeids and names for the menu
        // Code shamelessly stolen from the crop planting function!
        std::vector<itype_id> b_types;
        std::vector<std::string> b_names;
        for( auto &b : b_inv ) {
            if( std::find( b_types.begin(), b_types.end(), b->typeId() ) == b_types.end() ) {
                b_types.push_back( b->typeId() );
                b_names.push_back( item::nname( b->typeId() ) );
            }
        }
        // Choose brew from list
        int b_index = 0;
        if( b_types.size() > 1 ) {
            b_index = uilist( _( "Use which brew?" ), b_names );
        } else { //Only one brew type was in inventory, so it's automatically used
            if( !query_yn( _( "Set %s in the vat?" ), b_names[0] ) ) {
                b_index = -1;
            }
        }
        if( b_index < 0 ) {
            return;
        }
        to_deposit = true;
        brew_type = b_types[b_index];
        brew_nname = item::nname( brew_type );
    } else {
        item &brew = g->m.i_at( examp ).only_item();
        brew_type = brew.typeId();
        brew_nname = item::nname( brew_type );
        charges_on_ground = brew.charges;
        add_msg( _( "This keg contains %s (%d), %0.f%% full." ),
                 brew.tname(), brew.charges, brew.volume() * 100.0 / vat_volume );
        enum options { ADD_BREW, REMOVE_BREW, START_FERMENT };
        uilist selectmenu;
        selectmenu.text = _( "Select an action" );
        selectmenu.addentry( ADD_BREW, ( p.charges_of( brew_type ) > 0 ), MENU_AUTOASSIGN,
                             _( "Add more %s to the vat" ), brew_nname );
        selectmenu.addentry( REMOVE_BREW, brew.made_of( LIQUID ), MENU_AUTOASSIGN,
                             _( "Remove %s from the vat" ), brew.tname() );
        selectmenu.addentry( START_FERMENT, true, MENU_AUTOASSIGN, _( "Start fermenting cycle" ) );
        selectmenu.query();
        switch( selectmenu.ret ) {
            case ADD_BREW: {
                to_deposit = true;
                break;
            }
            case REMOVE_BREW: {
                liquid_handler::handle_liquid_from_ground( g->m.i_at( examp ).begin(), examp );
                return;
            }
            case START_FERMENT: {
                ferment = true;
                break;
            }
            default:
                add_msg( _( "Never mind." ) );
                return;
        }
    }
    if( to_deposit ) {
        item brew( brew_type, 0 );
        int charges_held = p.charges_of( brew_type );
        brew.charges = charges_on_ground;
        for( int i = 0; i < charges_held && !vat_full; i++ ) {
            p.use_charges( brew_type, 1 );
            brew.charges++;
            if( brew.volume() >= vat_volume ) {
                vat_full = true;
            }
        }
        add_msg( _( "Set %s in the vat." ), brew_nname );
        add_msg( _( "The keg now contains %s (%d), %0.f%% full." ),
                 brew.tname(), brew.charges, brew.volume() * 100.0 / vat_volume );
        g->m.i_clear( examp );
        //This is needed to bypass NOITEM
        g->m.add_item( examp, brew );
        p.moves -= to_moves<int>( 20_seconds );
        if( !vat_full ) {
            ferment = query_yn( _( "Start fermenting cycle?" ) );
        }
    }
    if( vat_full || ferment ) {
        g->m.i_at( examp ).only_item().set_age( 0_turns );
        g->m.furn_set( examp, f_fvat_full );
        if( vat_full ) {
            add_msg( _( "The vat is full, so you close the lid and start the fermenting cycle." ) );
        } else {
            add_msg( _( "You close the lid and start the fermenting cycle." ) );
        }
    }
}

void iexamine::fvat_full( player &p, const tripoint &examp )
{
    map_stack items_here = g->m.i_at( examp );
    if( items_here.empty() ) {
        debugmsg( "fvat_full was empty!" );
        g->m.furn_set( examp, f_fvat_empty );
        return;
    }

    for( item &it : items_here ) {
        if( !it.made_of_from_type( LIQUID ) ) {
            add_msg( _( "You remove %s from the vat." ), it.tname() );
            g->m.add_item_or_charges( p.pos(), it );
            g->m.i_rem( examp, &it );
        }
    }

    if( items_here.empty() ) {
        g->m.furn_set( examp, f_fvat_empty );
        return;
    }

    item &brew_i = *items_here.begin();
    // Does the vat contain unfermented brew, or already fermented booze?
    // TODO: Allow "recursive brewing" to continue without player having to check on it
    if( brew_i.is_brewable() ) {
        add_msg( _( "There's a vat of %s set to ferment there." ), brew_i.tname() );

        // TODO: change brew_time to return time_duration
        const time_duration brew_time = brew_i.brewing_time();
        const time_duration progress = brew_i.age();
        if( progress < brew_time ) {
            int hours = to_hours<int>( brew_time - progress );
            if( hours < 1 ) {
                add_msg( _( "It will finish brewing in less than an hour." ) );
            } else {
                add_msg( ngettext( "It will finish brewing in about %d hour.",
                                   "It will finish brewing in about %d hours.",
                                   hours ), hours );
            }
            return;
        }

        if( query_yn( _( "Finish brewing?" ) ) ) {
            const auto results = brew_i.brewing_results();

            g->m.i_clear( examp );
            for( const auto &result : results ) {
                // TODO: Different age based on settings
                item booze( result, brew_i.birthday(), brew_i.charges );
                g->m.add_item( examp, booze );
                if( booze.made_of_from_type( LIQUID ) ) {
                    add_msg( _( "The %s is now ready for bottling." ), booze.tname() );
                }
            }

            p.moves -= to_moves<int>( 5_seconds );
            p.practice( skill_cooking, std::min( to_minutes<int>( brew_time ) / 10, 100 ) );
        }

        return;
    } else {
        add_msg( _( "There's a vat of fermented %s there." ), brew_i.tname() );
    }

    const std::string booze_name = brew_i.tname();
    if( liquid_handler::handle_liquid_from_ground( items_here.begin(), examp ) ) {
        g->m.furn_set( examp, f_fvat_empty );
        add_msg( _( "You squeeze the last drops of %s from the vat." ), booze_name );
    }
}

static units::volume get_keg_capacity( const tripoint &pos )
{
    const furn_t &furn = g->m.furn( pos ).obj();
    return furn.keg_capacity;
}

/**
 * Check whether there is a keg on the map that can be filled via @ref pour_into_keg.
 */
bool iexamine::has_keg( const tripoint &pos )
{
    return get_keg_capacity( pos ) > 0_ml;
}

static void displace_items_except_one_liquid( const tripoint &examp )
{
    // Temporarily replace the real furniture with a fake furniture with NOITEM
    const furn_id previous_furn = g->m.furn( examp );
    g->m.furn_set( examp, furn_id( "f_no_item" ) );

    bool liquid_present = false;
    map_stack items = g->m.i_at( examp );
    for( map_stack::iterator it = items.begin(); it != items.end(); ) {
        if( !it->made_of_from_type( LIQUID ) || liquid_present ) {
            // This isn't a liquid or there was already another kind of liquid inside,
            // so this has to be moved.
            // This will add items to a space near the vat, because it's flagged as NOITEM.
            items.insert( *it );
            it = items.erase( it );
        } else {
            it++;
            liquid_present = true;
        }
    }

    // Replace the real furniture
    g->m.furn_set( examp, previous_furn );
}

void iexamine::keg( player &p, const tripoint &examp )
{
    none( p, examp );
    const auto keg_name = g->m.name( examp );
    units::volume keg_cap = get_keg_capacity( examp );

    const bool liquid_present = map_cursor( examp ).has_item_with( []( const item & it ) {
        return it.made_of_from_type( LIQUID );
    } );

    if( !liquid_present ) {
        add_msg( m_info, _( "It is empty." ) );
        // Get list of all drinks
        auto drinks_inv = p.items_with( []( const item & it ) {
            return it.made_of( LIQUID );
        } );
        if( drinks_inv.empty() ) {
            add_msg( m_info, _( "You don't have any drinks to fill the %s with." ), keg_name );
            return;
        }
        // Make lists of unique drinks... about third time we do this, maybe we ought to make a function next time
        std::vector<itype_id> drink_types;
        std::vector<std::string> drink_names;
        std::vector<double> drink_rot;
        for( auto &drink : drinks_inv ) {
            auto found_drink = std::find( drink_types.begin(), drink_types.end(), drink->typeId() );
            if( found_drink == drink_types.end() ) {
                drink_types.push_back( drink->typeId() );
                drink_names.push_back( item::nname( drink->typeId() ) );
                drink_rot.push_back( drink->get_relative_rot() );
            } else {
                auto rot_iter = std::next( drink_rot.begin(), std::distance( drink_types.begin(), found_drink ) );
                // Yep, worst rot wins.
                *rot_iter = std::max( *rot_iter, drink->get_relative_rot() );
            }
        }
        // Choose drink to store in keg from list
        int drink_index = 0;
        if( drink_types.size() > 1 ) {
            drink_index = uilist( _( "Store which drink?" ), drink_names );
            if( drink_index < 0 || static_cast<size_t>( drink_index ) >= drink_types.size() ) {
                drink_index = -1;
            }
        } else { //Only one drink type was in inventory, so it's automatically used
            if( !query_yn( _( "Fill the %1$s with %2$s?" ),
                           keg_name, drink_names[0].c_str() ) ) {
                drink_index = -1;
            }
        }
        if( drink_index < 0 ) {
            return;
        }

        // First empty the keg of foreign objects
        displace_items_except_one_liquid( examp );

        //Store liquid chosen in the keg
        itype_id drink_type = drink_types[ drink_index ];
        int charges_held = p.charges_of( drink_type );
        item drink( drink_type, 0 );
        drink.set_relative_rot( drink_rot[ drink_index ] );
        drink.charges = 0;
        bool keg_full = false;
        for( int i = 0; i < charges_held && !keg_full; i++ ) {
            g->u.use_charges( drink.typeId(), 1 );
            drink.charges++;
            keg_full = drink.volume() >= keg_cap;
        }
        if( keg_full ) {
            add_msg( _( "You completely fill the %1$s with %2$s." ),
                     keg_name, item::nname( drink_type ) );
        } else {
            add_msg( _( "You fill the %1$s with %2$s." ),
                     keg_name, item::nname( drink_type ) );
        }
        p.moves -= to_moves<int>( 10_seconds );
        g->m.i_clear( examp );
        g->m.add_item( examp, drink );
        return;
    } else {
        // First empty the keg of foreign objects
        displace_items_except_one_liquid( examp );

        map_stack items = g->m.i_at( examp );
        item &drink = items.only_item();
        const std::string drink_tname = drink.tname();
        const std::string drink_nname = item::nname( drink.typeId() );
        enum options {
            DISPENSE,
            HAVE_A_DRINK,
            REFILL,
            EXAMINE,
        };
        uilist selectmenu;
        selectmenu.addentry( DISPENSE, drink.made_of( LIQUID ), MENU_AUTOASSIGN,
                             _( "Dispense or dump %s" ), drink_tname );
        selectmenu.addentry( HAVE_A_DRINK, drink.is_food() && drink.made_of( LIQUID ),
                             MENU_AUTOASSIGN, _( "Have a drink" ) );
        selectmenu.addentry( REFILL, true, MENU_AUTOASSIGN, _( "Refill" ) );
        selectmenu.addentry( EXAMINE, true, MENU_AUTOASSIGN, _( "Examine" ) );

        selectmenu.text = _( "Select an action" );
        selectmenu.query();

        switch( selectmenu.ret ) {
            case DISPENSE:
                if( liquid_handler::handle_liquid_from_ground( items.begin(), examp ) ) {
                    add_msg( _( "You squeeze the last drops of %1$s from the %2$s." ),
                             drink_tname, keg_name );
                }
                return;

            case HAVE_A_DRINK:
                if( !p.eat( drink ) ) {
                    return; // They didn't actually drink
                }

                if( drink.charges == 0 ) {
                    add_msg( _( "You squeeze the last drops of %1$s from the %2$s." ),
                             drink_tname, keg_name );
                    g->m.i_clear( examp );
                }
                p.moves -= to_moves<int>( 5_seconds );
                return;

            case REFILL: {
                if( drink.volume() >= keg_cap ) {
                    add_msg( _( "The %s is completely full." ), keg_name );
                    return;
                }
                int charges_held = p.charges_of( drink.typeId() );
                if( charges_held < 1 ) {
                    add_msg( m_info, _( "You don't have any %1$s to fill the %2$s with." ),
                             drink_nname, keg_name );
                    return;
                }
                item tmp( drink.typeId(), calendar::turn, charges_held );
                pour_into_keg( examp, tmp );
                p.use_charges( drink.typeId(), charges_held - tmp.charges );
                add_msg( _( "You fill the %1$s with %2$s." ), keg_name, drink_nname );
                p.moves -= to_moves<int>( 10_seconds );
                return;
            }

            case EXAMINE: {
                add_msg( m_info, _( "It contains %s (%d), %0.f%% full." ),
                         drink_tname, drink.charges, drink.volume() * 100.0 / keg_cap );
                return;
            }

            default:
                return;
        }
    }
}

/**
 * Pour liquid into a keg (furniture) on the map. The transferred charges (if any)
 * will be removed from the liquid item.
 * @return Whether any charges have been transferred at all.
 */
bool iexamine::pour_into_keg( const tripoint &pos, item &liquid )
{
    const units::volume keg_cap = get_keg_capacity( pos );
    if( keg_cap <= 0_ml ) {
        return false;
    }
    const auto keg_name = g->m.name( pos );

    map_stack stack = g->m.i_at( pos );
    if( stack.empty() ) {
        g->m.add_item( pos, liquid );
        g->m.i_at( pos ).only_item().charges = 0; // Will be set later
    } else if( stack.only_item().typeId() != liquid.typeId() ) {
        add_msg( _( "The %s already contains some %s, you can't add a different liquid to it." ),
                 keg_name, item::nname( stack.only_item().typeId() ) );
        return false;
    }

    item &drink = stack.only_item();
    if( drink.volume() >= keg_cap ) {
        add_msg( _( "The %s is full." ), keg_name );
        return false;
    }

    add_msg( _( "You pour %1$s into the %2$s." ), liquid.tname(), keg_name );
    while( liquid.charges > 0 && drink.volume() < keg_cap ) {
        drink.charges++;
        liquid.charges--;
    }
    return true;
}

void pick_plant( player &p, const tripoint &examp,
                 const std::string &itemType, ter_id new_ter, bool seeds )
{
    bool auto_forage = get_option<bool>( "AUTO_FEATURES" ) &&
                       get_option<std::string>( "AUTO_FORAGING" ) != "off";
    if( p.is_player() && !auto_forage &&
        !query_yn( _( "Harvest the %s?" ), g->m.tername( examp ) ) ) {
        iexamine::none( p, examp );
        return;
    }

    const int survival = p.get_skill_level( skill_survival );
    p.practice( skill_survival, 6 );

    int plantBase = rng( 2, 5 );
    ///\EFFECT_SURVIVAL increases number of plants harvested
    int plantCount = rng( plantBase, plantBase + survival / 2 );
    plantCount = std::min( plantCount, 12 );

    g->m.spawn_item( p.pos(), itemType, plantCount, 0, calendar::turn );

    if( seeds ) {
        g->m.spawn_item( p.pos(), "seed_" + itemType, 1,
                         rng( plantCount / 4, plantCount / 2 ), calendar::turn );
    }

    g->m.ter_set( examp, new_ter );
}

void iexamine::tree_hickory( player &p, const tripoint &examp )
{
    if( harvest_common( p, examp, false, false ) ) {
        g->m.ter_set( examp, g->m.get_ter_transforms_into( examp ) );
    }
    if( !p.has_quality( quality_id( "DIG" ) ) ) {
        p.add_msg_if_player( m_info, _( "You have no tool to dig with..." ) );
        return;
    }
    if( p.is_player() &&
        !query_yn( _( "Dig up %s? This kills the tree!" ), g->m.tername( examp ) ) ) {
        return;
    }

    ///\EFFECT_SURVIVAL increases hickory root number per tree
    g->m.spawn_item( p.pos(), "hickory_root", rng( 1, 3 + p.get_skill_level( skill_survival ) ), 0,
                     calendar::turn );
    g->m.ter_set( examp, t_tree_hickory_dead );
    ///\EFFECT_SURVIVAL speeds up hickory root digging
    p.moves -= to_moves<int>( 20_seconds ) / ( p.get_skill_level( skill_survival ) + 1 ) + 100;
}

static item_location maple_tree_sap_container()
{
    const item maple_sap = item( "maple_sap", 0 );
    return g->inv_map_splice( [&]( const item & it ) {
        return it.get_remaining_capacity_for_liquid( maple_sap, true ) > 0;
    }, _( "Which container?" ), PICKUP_RANGE );
}

void iexamine::tree_maple( player &p, const tripoint &examp )
{
    if( !p.has_quality( quality_id( "DRILL" ) ) ) {
        add_msg( m_info, _( "You need a tool to drill the crust to tap this maple tree." ) );
        return;
    }

    if( !p.has_quality( quality_id( "HAMMER" ) ) ) {
        add_msg( m_info,
                 _( "You need a tool to hammer the spile into the crust to tap this maple tree." ) );
        return;
    }

    const inventory &crafting_inv = p.crafting_inventory();

    if( !crafting_inv.has_amount( "tree_spile", 1 ) ) {
        add_msg( m_info, _( "You need a %s to tap this maple tree." ),
                 item::nname( "tree_spile" ) );
        return;
    }

    std::vector<item_comp> comps;
    comps.push_back( item_comp( "tree_spile", 1 ) );
    p.consume_items( comps, 1, is_crafting_component );

    p.mod_moves( -to_moves<int>( 20_seconds ) );
    g->m.ter_set( examp, t_tree_maple_tapped );

    auto cont_loc = maple_tree_sap_container();

    item *container = cont_loc.get_item();
    if( container ) {
        g->m.add_item_or_charges( examp, *container, false );

        cont_loc.remove_item();
    } else {
        add_msg( m_info, _( "No container added. The sap will just spill on the ground." ) );
    }
}

void iexamine::tree_maple_tapped( player &p, const tripoint &examp )
{
    bool has_sap = false;
    item *container = nullptr;
    int charges = 0;

    const std::string maple_sap_name = item::nname( "maple_sap" );

    map_stack items = g->m.i_at( examp );
    if( !items.empty() ) {
        item &it = items.only_item();
        if( it.is_bucket() || it.is_watertight_container() ) {
            container = &it;

            if( !it.is_container_empty() && it.contents.front().typeId() == "maple_sap" ) {
                has_sap = true;
                charges = it.contents.front().charges;
            }
        }
    }

    enum options {
        REMOVE_TAP,
        ADD_CONTAINER,
        HARVEST_SAP,
        REMOVE_CONTAINER,
    };
    uilist selectmenu;
    selectmenu.addentry( REMOVE_TAP, true, MENU_AUTOASSIGN, _( "Remove tap" ) );
    selectmenu.addentry( ADD_CONTAINER, !container, MENU_AUTOASSIGN,
                         _( "Add a container to receive the %s" ), maple_sap_name );
    selectmenu.addentry( HARVEST_SAP, has_sap, MENU_AUTOASSIGN, _( "Harvest current %s (%d)" ),
                         maple_sap_name, charges );
    selectmenu.addentry( REMOVE_CONTAINER, container, MENU_AUTOASSIGN, _( "Remove container" ) );

    selectmenu.text = _( "Select an action" );
    selectmenu.query();

    switch( selectmenu.ret ) {
        case REMOVE_TAP: {
            if( !p.has_quality( quality_id( "HAMMER" ) ) ) {
                add_msg( m_info, _( "You need a hammering tool to remove the spile from the crust." ) );
                return;
            }

            item tree_spile( "tree_spile" );
            add_msg( _( "You remove the %s." ), tree_spile.tname( 1 ) );
            g->m.add_item_or_charges( p.pos(), tree_spile );

            if( container ) {
                g->m.add_item_or_charges( p.pos(), *container );
            }
            g->m.i_clear( examp );

            p.mod_moves( -to_moves<int>( 20_seconds ) );
            g->m.ter_set( examp, t_tree_maple );

            return;
        }

        case ADD_CONTAINER: {
            auto cont_loc = maple_tree_sap_container();

            container = cont_loc.get_item();
            if( container ) {
                g->m.add_item_or_charges( examp, *container, false );

                cont_loc.remove_item();
            } else {
                add_msg( m_info, _( "No container added. The sap will just spill on the ground." ) );
            }

            return;
        }

        case HARVEST_SAP: {
            liquid_handler::handle_liquid_from_container( *container, PICKUP_RANGE );
            return;
        }

        case REMOVE_CONTAINER: {
            g->u.assign_activity( activity_id( "ACT_PICKUP" ) );
            g->u.activity.targets.emplace_back( map_cursor( examp ), container );
            g->u.activity.values.push_back( 0 );
            return;
        }

        default:
            return;
    }
}

void iexamine::shrub_marloss( player &p, const tripoint &examp )
{
    if( p.has_trait( trait_THRESH_MYCUS ) ) {
        pick_plant( p, examp, "mycus_fruit", t_shrub_fungal );
    } else if( p.has_trait( trait_THRESH_MARLOSS ) ) {
        g->m.spawn_item( p.pos(), "mycus_fruit", 1, 0, calendar::turn );
        g->m.ter_set( examp, t_fungus );
        add_msg( m_info, _( "The shrub offers up a fruit, then crumbles into a fungal bed." ) );
    } else {
        pick_plant( p, examp, "marloss_berry", t_shrub_fungal );
    }
}

void iexamine::tree_marloss( player &p, const tripoint &examp )
{
    if( p.has_trait( trait_THRESH_MYCUS ) ) {
        pick_plant( p, examp, "mycus_fruit", t_tree_fungal );
        if( p.has_trait( trait_M_DEPENDENT ) && one_in( 3 ) ) {
            // Folks have a better shot at keeping fed.
            add_msg( m_info,
                     _( "We have located a particularly vital nutrient deposit underneath this location." ) );
            add_msg( m_good, _( "Additional nourishment is available." ) );
            g->m.ter_set( examp, t_marloss_tree );
        }
    } else if( p.has_trait( trait_THRESH_MARLOSS ) ) {
        g->m.spawn_item( p.pos(), "mycus_fruit", 1, 0, calendar::turn );
        g->m.ter_set( examp, t_tree_fungal );
        add_msg( m_info, _( "The tree offers up a fruit, then shrivels into a fungal tree." ) );
    } else {
        pick_plant( p, examp, "marloss_berry", t_tree_fungal );
    }
}

void iexamine::shrub_wildveggies( player &p, const tripoint &examp )
{
    // Ask if there's something possibly more interesting than this shrub here
    if( ( !g->m.i_at( examp ).empty() ||
          g->m.veh_at( examp ) ||
          !g->m.tr_at( examp ).is_null() ||
          g->critter_at( examp ) != nullptr ) &&
        !query_yn( _( "Forage through %s?" ), g->m.tername( examp ) ) ) {
        none( p, examp );
        return;
    }

    add_msg( _( "You forage through the %s." ), g->m.tername( examp ) );
    ///\EFFECT_SURVIVAL speeds up foraging
    int move_cost = 100000 / ( 2 * p.get_skill_level( skill_survival ) + 5 );
    ///\EFFECT_PER randomly speeds up foraging
    move_cost /= rng( std::max( 4, p.per_cur ), 4 + p.per_cur * 2 );
    p.assign_activity( activity_id( "ACT_FORAGE" ), move_cost, 0 );
    p.activity.placement = examp;
    p.activity.auto_resume = true;
    return;
}

void iexamine::recycle_compactor( player &, const tripoint &examp )
{
    // choose what metal to recycle
    auto metals = materials::get_compactable();
    uilist choose_metal;
    choose_metal.text = _( "Recycle what metal?" );
    for( auto &m : metals ) {
        choose_metal.addentry( m.name() );
    }
    choose_metal.query();
    int m_idx = choose_metal.ret;
    if( m_idx < 0 || m_idx >= static_cast<int>( metals.size() ) ) {
        add_msg( _( "Never mind." ) );
        return;
    }
    material_type m = metals.at( m_idx );

    // check inputs and tally total mass
    auto inputs = g->m.i_at( examp );
    units::mass sum_weight = 0_gram;
    auto ca = m.compact_accepts();
    std::set<material_id> accepts( ca.begin(), ca.end() );
    accepts.insert( m.id );
    for( auto &input : inputs ) {
        if( !input.only_made_of( accepts ) ) {
            //~ %1$s: an item in the compactor , %2$s: desired compactor output material
            add_msg( _( "You realize this isn't going to work because %1$s is not made purely of %2$s." ),
                     input.tname(), m.name() );
            return;
        }
        if( input.is_container() && !input.is_container_empty() ) {
            //~ %1$s: an item in the compactor
            add_msg( _( "You realize this isn't going to work because %1$s has not been emptied of its contents." ),
                     input.tname() );
            return;
        }
        sum_weight += input.weight();
    }
    if( sum_weight <= 0_gram ) {
        //~ %1$s: desired compactor output material
        add_msg( _( "There is no %1$s in the compactor.  Drop some metal items onto it and try again." ),
                 m.name() );
        return;
    }

    // See below for recover_factor (rng(6,9)/10), this
    // is the normal value of that recover factor.
    static const double norm_recover_factor = 8.0 / 10.0;
    const units::mass norm_recover_weight = sum_weight * norm_recover_factor;

    // choose output
    uilist choose_output;
    //~ %1$.3f: total mass of material in compactor, %2$s: weight units , %3$s: compactor output material
    choose_output.text = string_format( _( "Compact %1$.3f %2$s of %3$s into:" ),
                                        convert_weight( sum_weight ), weight_units(), m.name() );
    for( auto &ci : m.compacts_into() ) {
        auto it = item( ci, 0, item::solitary_tag{} );
        const int amount = norm_recover_weight / it.weight();
        //~ %1$d: number of, %2$s: output item
        choose_output.addentry( string_format( _( "about %1$d %2$s" ), amount,
                                               it.tname( amount ) ) );
    }
    choose_output.query();
    int o_idx = choose_output.ret;
    if( o_idx < 0 || o_idx >= static_cast<int>( m.compacts_into().size() ) ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    // remove items
    for( auto it = inputs.begin(); it != inputs.end(); ) {
        it = inputs.erase( it );
    }

    // produce outputs
    double recover_factor = rng( 6, 9 ) / 10.0;
    sum_weight = sum_weight * recover_factor;
    sounds::sound( examp, 80, sounds::sound_t::combat, _( "Ka-klunk!" ), true, "tool", "compactor" );
    bool out_desired = false;
    bool out_any = false;
    for( auto it = m.compacts_into().begin() + o_idx; it != m.compacts_into().end(); ++it ) {
        const units::mass ow = item( *it, 0, item::solitary_tag{} ).weight();
        int count = sum_weight / ow;
        sum_weight -= count * ow;
        if( count > 0 ) {
            g->m.spawn_item( examp, *it, count, 1, calendar::turn );
            if( !out_any ) {
                out_any = true;
                if( it == m.compacts_into().begin() + o_idx ) {
                    out_desired = true;
                }
            }
        }
    }

    // feedback to user
    if( !out_any ) {
        add_msg( _( "The compactor chews up all the items in its hopper." ) );
        //~ %1$s: compactor output material
        add_msg( _( "The compactor beeps: \"No %1$s to process!\"" ), m.name() );
        return;
    }
    if( !out_desired ) {
        //~ %1$s: compactor output material
        add_msg( _( "The compactor beeps: \"Insufficient %1$s!\"" ), m.name() );
        add_msg( _( "It spits out an assortment of smaller pieces instead." ) );
    }
}

void iexamine::trap( player &p, const tripoint &examp )
{
    const auto &tr = g->m.tr_at( examp );
    if( !p.is_player() || tr.is_null() ) {
        return;
    }
    const int possible = tr.get_difficulty();
    bool seen = tr.can_see( examp, p );

    if( tr.loadid == tr_unfinished_construction || g->m.partial_con_at( examp ) ) {
        partial_con *pc = g->m.partial_con_at( examp );
        if( pc ) {
            const std::vector<construction> &list_constructions = get_constructions();
            const construction &built = list_constructions[pc->id];
            if( !query_yn( _( "Unfinished task: %s, %d%% complete here, continue construction?" ),
                           built.description, pc->counter / 100000 ) ) {
                if( query_yn( _( "Cancel construction?" ) ) ) {
                    g->m.disarm_trap( examp );
                    for( const item &it : pc->components ) {
                        g->m.add_item_or_charges( g->u.pos(), it );
                    }
                    g->m.partial_con_remove( examp );
                    return;
                } else {
                    return;
                }
            } else {
                g->u.assign_activity( activity_id( "ACT_BUILD" ) );
                g->u.activity.placement = g->m.getabs( examp );
                return;
            }
        } else {
            return;
        }
    }
    if( seen && possible >= 99 ) {
        add_msg( m_info, _( "That %s looks too dangerous to mess with. Best leave it alone." ),
                 tr.name() );
        return;
    }
    // Some traps are not actual traps. Those should get a different query.
    if( seen && possible == 0 &&
        tr.get_avoidance() == 0 ) { // Separated so saying no doesn't trigger the other query.
        if( query_yn( _( "There is a %s there. Take down?" ), tr.name() ) ) {
            g->m.disarm_trap( examp );
        }
    } else if( seen && query_yn( _( "There is a %s there.  Disarm?" ), tr.name() ) ) {
        g->m.disarm_trap( examp );
    }
}

void iexamine::water_source( player &p, const tripoint &examp )
{
    item water = g->m.water_from( examp );
    ( void ) p; // TODO: use me
    liquid_handler::handle_liquid( water, nullptr, 0, &examp );
}

void iexamine::clean_water_source( player &, const tripoint &examp )
{
    item water = item( "water_clean", 0, item::INFINITE_CHARGES );
    liquid_handler::handle_liquid( water, nullptr, 0, &examp );
}

const itype *furn_t::crafting_pseudo_item_type() const
{
    if( crafting_pseudo_item.empty() ) {
        return nullptr;
    }
    return item::find_type( crafting_pseudo_item );
}

const itype *furn_t::crafting_ammo_item_type() const
{
    const itype *pseudo = crafting_pseudo_item_type();
    if( pseudo->tool && !pseudo->tool->ammo_id.empty() ) {
        return item::find_type( ammotype( *pseudo->tool->ammo_id.begin() )->default_ammotype() );
    }
    return nullptr;
}

static int count_charges_in_list( const itype *type, const map_stack &items )
{
    for( const auto &candidate : items ) {
        if( candidate.type == type ) {
            return candidate.charges;
        }
    }
    return 0;
}

void iexamine::reload_furniture( player &p, const tripoint &examp )
{
    const furn_t &f = g->m.furn( examp ).obj();
    const itype *type = f.crafting_pseudo_item_type();
    const itype *ammo = f.crafting_ammo_item_type();
    if( type == nullptr || ammo == nullptr ) {
        add_msg( m_info, _( "This %s can not be reloaded!" ), f.name() );
        return;
    }
    map_stack items_here = g->m.i_at( examp );
    const int amount_in_furn = count_charges_in_list( ammo, items_here );
    const int amount_in_inv = p.charges_of( ammo->get_id() );
    if( amount_in_furn > 0 ) {
        if( p.query_yn( _( "The %1$s contains %2$d %3$s.  Unload?" ), f.name(), amount_in_furn,
                        ammo->nname( amount_in_furn ) ) ) {
            p.assign_activity( activity_id( "ACT_PICKUP" ) );
            p.activity.targets.emplace_back( map_cursor( examp ), &g->m.i_at( examp ).only_item() );
            p.activity.values.push_back( 0 );
            return;
        }
    }

    const int max_amount_in_furn = ammo->charges_per_volume( f.max_volume );
    const int max_reload_amount = max_amount_in_furn - amount_in_furn;
    if( max_reload_amount <= 0 ) {
        return;
    }
    if( amount_in_inv == 0 ) {
        //~ Reloading or restocking a piece of furniture, for example a forge.
        add_msg( m_info, _( "You need some %1$s to reload this %2$s." ), ammo->nname( 2 ),
                 f.name() );
        return;
    }
    const int max_amount = std::min( amount_in_inv, max_reload_amount );
    //~ Loading fuel or other items into a piece of furniture.
    const std::string popupmsg = string_format( _( "Put how many of the %1$s into the %2$s?" ),
                                 ammo->nname( max_amount ), f.name() );
    int amount = string_input_popup()
                 .title( popupmsg )
                 .width( 20 )
                 .text( to_string( max_amount ) )
                 .only_digits( true )
                 .query_int();
    if( amount <= 0 || amount > max_amount ) {
        return;
    }
    p.use_charges( ammo->get_id(), amount );
    auto items = g->m.i_at( examp );
    for( auto &itm : items ) {
        if( itm.type == ammo ) {
            itm.charges += amount;
            amount = 0;
            break;
        }
    }
    if( amount != 0 ) {
        item it( ammo, calendar::turn, amount );
        g->m.add_item( examp, it );
    }

    const int amount_in_furn_after_placing = count_charges_in_list( ammo, items );
    //~ %1$s - furniture, %2$d - number, %3$s items.
    add_msg( _( "The %1$s contains %2$d %3$s." ), f.name(), amount_in_furn_after_placing,
             ammo->nname( amount_in_furn_after_placing ) );

    add_msg( _( "You reload the %s." ), g->m.furnname( examp ) );
    p.moves -= to_moves<int>( 5_seconds );
}

void iexamine::curtains( player &p, const tripoint &examp )
{
    const bool closed_window_with_curtains = g->m.has_flag( "BARRICADABLE_WINDOW_CURTAINS", examp ) ;
    if( g->m.is_outside( p.pos() ) && ( g->m.has_flag( "WALL", examp ) ||
                                        closed_window_with_curtains ) ) {
        locked_object( p, examp );
        return;
    }

    const ter_id ter = g->m.ter( examp );

    // Peek through the curtains, or tear them down.
    uilist window_menu;
    window_menu.text = _( "Do what with the curtains?" );
    window_menu.addentry( 0, ( !ter.obj().close &&
                               closed_window_with_curtains ), 'p', _( "Peek through the closed curtains." ) );
    window_menu.addentry( 1, true, 't', _( "Tear down the curtains." ) );
    window_menu.query();
    const int choice = window_menu.ret;

    if( choice == 0 ) {
        // Peek
        g->peek( examp );
        p.add_msg_if_player( _( "You carefully peek through the curtains." ) );
    } else if( choice == 1 ) {
        // Mr. Gorbachev, tear down those curtains!
        if( ter == t_window_domestic || ter == t_curtains ) {
            g->m.ter_set( examp, t_window_no_curtains );
        } else if( ter == t_window_open ) {
            g->m.ter_set( examp, t_window_no_curtains_open );
        } else if( ter == t_window_domestic_taped ) {
            g->m.ter_set( examp, t_window_no_curtains_taped );
        }

        g->m.spawn_item( p.pos(), "nail", 1, 4, calendar::turn );
        g->m.spawn_item( p.pos(), "sheet", 2, 0, calendar::turn );
        g->m.spawn_item( p.pos(), "stick", 1, 0, calendar::turn );
        g->m.spawn_item( p.pos(), "string_36", 1, 0, calendar::turn );
        p.moves -= to_moves<int>( 10_seconds );
        p.add_msg_if_player( _( "You tear the curtains and curtain rod off the windowframe." ) );
    } else {
        p.add_msg_if_player( _( "Never mind." ) );
    }
}

void iexamine::sign( player &p, const tripoint &examp )
{
    std::string existing_signage = g->m.get_signage( examp );
    bool previous_signage_exists = !existing_signage.empty();

    // Display existing message, or lack thereof.
    if( p.has_trait( trait_ILLITERATE ) ) {
        popup( _( "You're illiterate, and can't read the message on the sign." ) );
    } else if( previous_signage_exists ) {
        popup( existing_signage.c_str() );
    } else {
        p.add_msg_if_player( m_neutral, _( "Nothing legible on the sign." ) );
    }

    // Allow chance to modify message.
    std::vector<tool_comp> tools;
    std::vector<const item *> filter = p.crafting_inventory().items_with( []( const item & it ) {
        return it.has_flag( "WRITE_MESSAGE" ) && it.charges > 0;
    } );
    tools.reserve( filter.size() );
    for( const item *writing_item : filter ) {
        tools.push_back( tool_comp( writing_item->typeId(), 1 ) );
    }

    if( !tools.empty() ) {
        // Different messages if the sign already has writing associated with it.
        std::string query_message = previous_signage_exists ?
                                    _( "Overwrite the existing message on the sign?" ) :
                                    _( "Add a message to the sign?" );
        std::string ignore_message = _( "You leave the sign alone." );
        if( query_yn( query_message ) ) {
            std::string signage = string_input_popup()
                                  .title( _( "Write what?" ) )
                                  .identifier( "signage" )
                                  .query_string();
            if( signage.empty() ) {
                p.add_msg_if_player( m_neutral, ignore_message );
            } else {
                std::string spray_painted_message = previous_signage_exists ?
                                                    _( "You overwrite the previous message on the sign with your graffiti." ) :
                                                    _( "You graffiti a message onto the sign." );
                g->m.set_signage( examp, signage );
                p.add_msg_if_player( m_info, spray_painted_message );
                p.mod_moves( - 20 * signage.length() );
                p.consume_tools( tools, 1 );
            }
        } else {
            p.add_msg_if_player( m_neutral, ignore_message );
        }
    }
}

static int getNearPumpCount( const tripoint &p )
{
    int result = 0;
    for( const tripoint &tmp : g->m.points_in_radius( p, 12 ) ) {
        const auto t = g->m.ter( tmp );
        if( t == ter_str_id( "t_gas_pump" ) || t == ter_str_id( "t_gas_pump_a" ) ) {
            result++;
        }
    }
    return result;
}

static cata::optional<tripoint> getNearFilledGasTank( const tripoint &center, int &gas_units )
{
    cata::optional<tripoint> tank_loc;
    int distance = INT_MAX;
    gas_units = 0;

    for( const tripoint &tmp : g->m.points_in_radius( center, SEEX * 2 ) ) {
        if( g->m.ter( tmp ) != ter_str_id( "t_gas_tank" ) ) {
            continue;
        }

        const int new_distance = rl_dist( center, tmp );

        if( new_distance >= distance ) {
            continue;
        }
        if( !tank_loc ) {
            // Return a potentially empty tank, but only if we don't find a closer full one.
            tank_loc.emplace( tmp );
        }
        for( auto &k : g->m.i_at( tmp ) ) {
            if( k.made_of( LIQUID ) ) {
                distance = new_distance;
                tank_loc.emplace( tmp );
                gas_units = k.charges;
                break;
            }
        }
    }
    return tank_loc;
}

static int getGasDiscountCardQuality( const item &it )
{
    std::set<std::string> tags = it.type->item_tags;

    for( const std::string &tag : tags ) {

        if( tag.size() > 15 && tag.substr( 0, 15 ) == "DISCOUNT_VALUE_" ) {
            return atoi( tag.substr( 15 ).c_str() );
        }
    }

    return 0;
}

static int findBestGasDiscount( player &p )
{
    int discount = 0;

    for( size_t i = 0; i < p.inv.size(); i++ ) {
        item &it = p.inv.find_item( i );

        if( it.has_flag( "GAS_DISCOUNT" ) ) {

            int q = getGasDiscountCardQuality( it );
            if( q > discount ) {
                discount = q;
            }
        }
    }

    return discount;
}

static std::string str_to_illiterate_str( std::string s )
{
    if( !g->u.has_trait( trait_ILLITERATE ) ) {
        return s;
    } else {
        for( auto &i : s ) {
            i = i + rng( 0, 5 ) - rng( 0, 5 );
            if( i < ' ' ) {
                // some control character, most likely not handled correctly be the print functions
                i = ' ';
            } else if( i == '%' ) {
                // avoid characters that trigger formatting in the various print functions
                i++;
            }
        }
        return s;
    }
}

static std::string getGasDiscountName( int discount )
{
    if( discount == 3 ) {
        return str_to_illiterate_str( _( "Platinum member" ) );
    } else if( discount == 2 ) {
        return str_to_illiterate_str( _( "Gold member" ) );
    } else if( discount == 1 ) {
        return str_to_illiterate_str( _( "Silver member" ) );
    } else {
        return str_to_illiterate_str( _( "Beloved customer" ) );
    }
}

static int getGasPricePerLiter( int discount )
{
    // Those prices are in cents
    static const int prices[4] = { 1400, 1320, 1200, 1000 };
    if( discount < 0 || discount > 3 ) {
        return prices[0];
    } else {
        return prices[discount];
    }
}

static cata::optional<tripoint> getGasPumpByNumber( const tripoint &p, int number )
{
    int k = 0;
    for( const tripoint &tmp : g->m.points_in_radius( p, 12 ) ) {
        const auto t = g->m.ter( tmp );
        if( ( t == ter_str_id( "t_gas_pump" ) || t == ter_str_id( "t_gas_pump_a" ) ) && number == k++ ) {
            return tmp;
        }
    }
    return cata::nullopt;
}

static bool toPumpFuel( const tripoint &src, const tripoint &dst, int units )
{
    auto items = g->m.i_at( src );
    for( auto item_it = items.begin(); item_it != items.end(); ++item_it ) {
        if( item_it->made_of( LIQUID ) ) {
            if( item_it->charges < units ) {
                return false;
            }

            item_it->charges -= units;

            item liq_d( item_it->type, calendar::turn, units );

            const auto backup_pump = g->m.ter( dst );
            g->m.ter_set( dst, ter_str_id::NULL_ID() );
            g->m.add_item_or_charges( dst, liq_d );
            g->m.ter_set( dst, backup_pump );

            if( item_it->charges < 1 ) {
                items.erase( item_it );
            }

            return true;
        }
    }

    return false;
}

static int fromPumpFuel( const tripoint &dst, const tripoint &src )
{
    auto items = g->m.i_at( src );
    for( auto item_it = items.begin(); item_it != items.end(); ++item_it ) {
        if( item_it->made_of( LIQUID ) ) {
            // how much do we have in the pump?
            item liq_d( item_it->type, calendar::turn, item_it->charges );

            // add the charges to the destination
            const auto backup_tank = g->m.ter( dst );
            g->m.ter_set( dst, ter_str_id::NULL_ID() );
            g->m.add_item_or_charges( dst, liq_d );
            g->m.ter_set( dst, backup_tank );

            // remove the liquid from the pump
            int amount = item_it->charges;
            items.erase( item_it );
            return amount;
        }
    }
    return -1;
}

static void turnOnSelectedPump( const tripoint &p, int number )
{
    int k = 0;
    for( const tripoint &tmp : g->m.points_in_radius( p, 12 ) ) {
        const auto t = g->m.ter( tmp );
        if( t == ter_str_id( "t_gas_pump" ) || t == ter_str_id( "t_gas_pump_a" ) ) {
            if( number == k++ ) {
                g->m.ter_set( tmp, ter_str_id( "t_gas_pump_a" ) );
            } else {
                g->m.ter_set( tmp, ter_str_id( "t_gas_pump" ) );
            }
        }
    }
}

void iexamine::pay_gas( player &p, const tripoint &examp )
{

    int choice = -1;
    const int buy_gas = 1;
    const int choose_pump = 2;
    const int hack = 3;
    const int refund = 4;

    if( p.has_trait( trait_ILLITERATE ) ) {
        popup( _( "You're illiterate, and can't read the screen." ) );
    }

    int pumpCount = getNearPumpCount( examp );
    if( pumpCount == 0 ) {
        popup( str_to_illiterate_str( _( "Failure! No gas pumps found!" ) ) );
        return;
    }

    int tankGasUnits;
    const cata::optional<tripoint> pTank_ = getNearFilledGasTank( examp, tankGasUnits );
    if( !pTank_ ) {
        popup( str_to_illiterate_str( _( "Failure! No gas tank found!" ) ) );
        return;
    }
    const tripoint pTank = *pTank_;

    if( tankGasUnits == 0 ) {
        popup( str_to_illiterate_str(
                   _( "This station is out of fuel.  We apologize for the inconvenience." ) ) );
        return;
    }

    if( uistate.ags_pay_gas_selected_pump + 1 > pumpCount ) {
        uistate.ags_pay_gas_selected_pump = 0;
    }

    int discount = findBestGasDiscount( p );
    std::string discountName = getGasDiscountName( discount );

    int pricePerUnit = getGasPricePerLiter( discount );

    bool can_hack = ( !p.has_trait( trait_ILLITERATE ) && ( ( p.has_charges( "electrohack", 25 ) ) ||
                      ( p.has_bionic( bionic_id( "bio_fingerhack" ) ) && p.power_level > 24 ) ) );

    uilist amenu;
    amenu.selected = 1;
    amenu.text = str_to_illiterate_str( _( "Welcome to AutoGas!" ) );
    amenu.addentry( 0, false, -1, str_to_illiterate_str( _( "What would you like to do?" ) ) );

    amenu.addentry( buy_gas, true, 'b', str_to_illiterate_str( _( "Buy gas." ) ) );
    amenu.addentry( refund, true, 'r', str_to_illiterate_str( _( "Refund cash." ) ) );

    std::string gaspumpselected = str_to_illiterate_str( _( "Current gas pump: " ) ) +
                                  to_string( uistate.ags_pay_gas_selected_pump + 1 );
    amenu.addentry( 0, false, -1, gaspumpselected );
    amenu.addentry( choose_pump, true, 'p', str_to_illiterate_str( _( "Choose a gas pump." ) ) );

    amenu.addentry( 0, false, -1, str_to_illiterate_str( _( "Your discount: " ) ) + discountName );
    amenu.addentry( 0, false, -1, str_to_illiterate_str( _( "Your price per gasoline unit: " ) ) +
                    format_money( pricePerUnit ) );

    if( can_hack ) {
        amenu.addentry( hack, true, 'h', _( "Hack console." ) );
    }

    amenu.query();
    choice = amenu.ret;

    if( choose_pump == choice ) {
        uilist amenu;
        amenu.selected = uistate.ags_pay_gas_selected_pump;
        amenu.text = str_to_illiterate_str( _( "Please choose gas pump:" ) );

        for( int i = 0; i < pumpCount; i++ ) {
            amenu.addentry( i, true, -1,
                            str_to_illiterate_str( _( "Pump " ) ) + to_string( i + 1 ) );
        }
        amenu.query();
        choice = amenu.ret;

        if( choice < 0 ) {
            return;
        }

        uistate.ags_pay_gas_selected_pump = choice;

        turnOnSelectedPump( examp, uistate.ags_pay_gas_selected_pump );

        return;

    }

    if( buy_gas == choice ) {
        int money = p.charges_of( "cash_card" );

        if( money < pricePerUnit ) {
            popup( str_to_illiterate_str(
                       _( "Not enough money, please refill your cash card." ) ) ); //or ride on a solar car, ha ha ha
            return;
        }

        int maximum_liters = std::min( money / pricePerUnit, tankGasUnits / 1000 );

        std::string popupmsg = string_format(
                                   _( "How many liters of gasoline to buy? Max: %d L. (0 to cancel) " ), maximum_liters );
        int liters = string_input_popup()
                     .title( popupmsg )
                     .width( 20 )
                     .text( to_string( maximum_liters ) )
                     .only_digits( true )
                     .query_int();
        if( liters <= 0 ) {
            return;
        }
        if( liters > maximum_liters ) {
            liters = maximum_liters;
        }

        const cata::optional<tripoint> pGasPump = getGasPumpByNumber( examp,
                uistate.ags_pay_gas_selected_pump );
        if( !pGasPump || !toPumpFuel( pTank, *pGasPump, liters * 1000 ) ) {
            return;
        }

        sounds::sound( p.pos(), 6, sounds::sound_t::activity, _( "Glug Glug Glug" ), true, "tool",
                       "gaspump" );

        int cost = liters * pricePerUnit;
        money -= cost;
        p.use_charges( "cash_card", cost );

        add_msg( m_info, _( "Your cash cards now hold %s." ), format_money( money ) );
        p.moves -= to_moves<int>( 5_seconds );
        return;
    }

    if( hack == choice ) {
        switch( hack_attempt( p ) ) {
            case HACK_UNABLE:
                break;
            case HACK_FAIL:
                g->events().send<event_type::triggers_alarm>( p.getID() );
                sounds::sound( p.pos(), 60, sounds::sound_t::music, _( "an alarm sound!" ), true, "environment",
                               "alarm" );
                if( examp.z > 0 && !g->timed_events.queued( TIMED_EVENT_WANTED ) ) {
                    g->timed_events.add( TIMED_EVENT_WANTED, calendar::turn + 30_minutes, 0,
                                         p.global_sm_location() );
                }
                break;
            case HACK_NOTHING:
                add_msg( _( "Nothing happens." ) );
                break;
            case HACK_SUCCESS:
                const cata::optional<tripoint> pGasPump = getGasPumpByNumber( examp,
                        uistate.ags_pay_gas_selected_pump );
                if( pGasPump && toPumpFuel( pTank, *pGasPump, tankGasUnits ) ) {
                    add_msg( _( "You hack the terminal and route all available fuel to your pump!" ) );
                    sounds::sound( p.pos(), 6, sounds::sound_t::activity,
                                   _( "Glug Glug Glug Glug Glug Glug Glug Glug Glug" ), true, "tool", "gaspump" );
                } else {
                    add_msg( _( "Nothing happens." ) );
                }
                break;
        }
    }

    if( refund == choice ) {
        const int pos = p.inv.position_by_type( itype_id( "cash_card" ) );

        if( pos == INT_MIN ) {
            add_msg( _( "Never mind." ) );
            return;
        }

        item *cashcard = &( p.i_at( pos ) );
        // Okay, we have a cash card. Now we need to know what's left in the pump.
        const cata::optional<tripoint> pGasPump = getGasPumpByNumber( examp,
                uistate.ags_pay_gas_selected_pump );
        int amount = pGasPump ? fromPumpFuel( pTank, *pGasPump ) : 0;
        if( amount >= 0 ) {
            sounds::sound( p.pos(), 6, sounds::sound_t::activity, _( "Glug Glug Glug" ), true, "tool",
                           "gaspump" );
            cashcard->charges += amount * pricePerUnit / 1000.0f;
            add_msg( m_info, _( "Your cash cards now hold %s." ), format_money( p.charges_of( "cash_card" ) ) );
            p.moves -= to_moves<int>( 5_seconds );
            return;
        } else {
            popup( _( "Unable to refund, no fuel in pump." ) );
            return;
        }
    }
}

void iexamine::ledge( player &p, const tripoint &examp )
{

    uilist cmenu;
    cmenu.text = _( "There is a ledge here.  What do you want to do?" );
    cmenu.addentry( 1, true, 'j', _( "Jump over." ) );
    cmenu.addentry( 2, true, 'c', _( "Climb down." ) );
    cmenu.query();

    switch( cmenu.ret ) {
        case 1: {
            tripoint dest( p.posx() + 2 * sgn( examp.x - p.posx() ), p.posy() + 2 * sgn( examp.y - p.posy() ),
                           p.posz() );
            if( p.get_str() < 4 ) {
                add_msg( m_warning, _( "You are too weak to jump over an obstacle." ) );
            } else if( 100 * p.weight_carried() / p.weight_capacity() > 25 ) {
                add_msg( m_warning, _( "You are too burdened to jump over an obstacle." ) );
            } else if( !g->m.valid_move( examp, dest, false, true ) ) {
                add_msg( m_warning, _( "You cannot jump over an obstacle - something is blocking the way." ) );
            } else if( g->critter_at( dest ) ) {
                add_msg( m_warning, _( "You cannot jump over an obstacle - there is %s blocking the way." ),
                         g->critter_at( dest )->disp_name() );
            } else if( g->m.ter( dest ).obj().trap == tr_ledge ) {
                add_msg( m_warning, _( "You are not going to jump over an obstacle only to fall down." ) );
            } else {
                add_msg( m_info, _( "You jump over an obstacle." ) );
                p.increase_activity_level( LIGHT_EXERCISE );
                p.setpos( dest );
            }
            break;
        }
        case 2: {
            if( !g->m.has_zlevels() ) {
                // No climbing down in 2D mode
                return;
            }

            if( !g->m.valid_move( p.pos(), examp, false, true ) ) {
                // Covered with something
                return;
            }

            tripoint where = examp;
            tripoint below = examp;
            below.z--;
            while( g->m.valid_move( where, below, false, true ) ) {
                where.z--;
                below.z--;
            }

            const int height = examp.z - where.z;
            if( height == 0 ) {
                p.add_msg_if_player( _( "You can't climb down there" ) );
                return;
            }

            const int climb_cost = p.climbing_cost( where, examp );
            const auto fall_mod = p.fall_damage_mod();
            std::string query_str = ngettext( "Looks like %d story. Jump down?",
                                              "Looks like %d stories. Jump down?",
                                              height );
            if( height > 1 && !query_yn( query_str.c_str(), height ) ) {
                return;
            } else if( height == 1 ) {
                std::string query;
                p.increase_activity_level( MODERATE_EXERCISE );
                if( climb_cost <= 0 && fall_mod > 0.8 ) {
                    query = _( "You probably won't be able to get up and jumping down may hurt. Jump?" );
                } else if( climb_cost <= 0 ) {
                    query = _( "You probably won't be able to get back up. Climb down?" );
                } else if( climb_cost < 200 ) {
                    query = _( "You should be able to climb back up easily if you climb down there. Climb down?" );
                } else {
                    query = _( "You may have problems climbing back up. Climb down?" );
                }

                if( !query_yn( query.c_str() ) ) {
                    return;
                }
            }

            p.moves -= to_moves<int>( 1_seconds + 1_seconds * fall_mod );
            p.setpos( examp );
            if( climb_cost > 0 || rng_float( 0.8, 1.0 ) > fall_mod ) {
                // One tile of falling less (possibly zero)
                g->vertical_move( -1, true );
            }
            g->m.creature_on_trap( p );
            break;
        }
        default:
            popup( _( "You decided to step back from the ledge." ) );
            break;
    }
}

static player &player_on_couch( player &p, const tripoint &autodoc_loc, player &null_patient,
                                bool &adjacent_couch, tripoint &couch_pos )
{
    for( const auto &couch_loc : g->m.points_in_radius( autodoc_loc, 1, 0 ) ) {
        const furn_str_id couch( "f_autodoc_couch" );
        if( g->m.furn( couch_loc ) == couch ) {
            adjacent_couch = true;
            couch_pos = couch_loc;
            if( p.pos() == couch_loc ) {
                return p;
            }
            for( const npc *e : g->allies() ) {
                if( e->pos() == couch_loc ) {
                    return  *g->critter_by_id<player>( e->getID() );
                }
            }
        }
    }
    return null_patient;
}

static item &cyborg_on_couch( const tripoint &couch_pos, item &null_cyborg )
{
    for( item &it : g->m.i_at( couch_pos ) ) {
        if( it.typeId() == "bot_broken_cyborg" || it.typeId() == "bot_prototype_cyborg" ) {
            return it;
        }
        if( it.typeId() == "corpse" ) {
            if( it.get_mtype()->id == "mon_broken_cyborg" || it.get_mtype()->id == "mon_prototype_cyborg" ) {
                return it;
            }
        }
    }
    return null_cyborg;
}

static player &best_installer( player &p, player &null_player, int difficulty )
{
    float player_skill = p.bionics_adjusted_skill( skill_firstaid,
                         skill_computer,
                         skill_electronics );

    std::vector< std::pair<float, int>> ally_skills;
    ally_skills.reserve( g->allies().size() );
    for( size_t i = 0; i < g->allies().size() ; i ++ ) {
        std::pair<float, int> ally_skill;
        const npc *e = g->allies()[ i ];

        player &ally = *g->critter_by_id<player>( e->getID() );
        ally_skill.second = i;
        ally_skill.first = ally.bionics_adjusted_skill( skill_firstaid,
                           skill_computer,
                           skill_electronics );
        ally_skills.push_back( ally_skill );
    }
    std::sort( ally_skills.begin(), ally_skills.end(), [&]( const std::pair<float, int> &lhs,
    const std::pair<float, int> &rhs ) {
        return rhs.first < lhs.first;
    } );
    int player_cos = bionic_manip_cos( player_skill, true, difficulty );
    for( size_t i = 0; i < g->allies().size() ; i ++ ) {
        if( ally_skills[ i ].first > player_skill ) {
            const npc *e = g->allies()[ ally_skills[ i ].second ];
            player &ally = *g->critter_by_id<player>( e->getID() );
            int ally_cos = bionic_manip_cos( ally_skills[ i ].first, true, difficulty );
            if( e->has_effect( effect_sleep ) ) {
                if( !g->u.query_yn(
                        //~ %1$s is the name of the ally
                        _( "<color_white>%1$s is asleep, but has a <color_green>%2$d<color_white> chance of success compared to your <color_red>%3$d<color_white> chance of success.  Continue with a higher risk of failure?</color>" ),
                        ally.disp_name(), ally_cos, player_cos ) ) {
                    return null_player;
                } else {
                    continue;
                }
            }
            //~ %1$s is the name of the ally
            add_msg( _( "%1$s will perform the operation with a %2$d chance of success." ), ally.disp_name(),
                     ally_cos );
            return ally;
        } else {
            break;
        }
    }

    return p;
}

void iexamine::autodoc( player &p, const tripoint &examp )
{
    enum options {
        INSTALL_CBM,
        UNINSTALL_CBM,
        BONESETTING,
    };

    bool adjacent_couch = false;
    static avatar null_player;
    tripoint couch_pos;
    player &patient = player_on_couch( p, examp, null_player, adjacent_couch, couch_pos );

    static item null_cyborg;
    item &cyborg = cyborg_on_couch( couch_pos, null_cyborg );

    if( !adjacent_couch ) {
        popup( _( "No connected couches found.  Operation impossible.  Exiting." ) );
        return;
    }
    if( &patient == &null_player ) {
        if( &cyborg != &null_cyborg ) {
            if( cyborg.typeId() == "corpse" && !cyborg.active ) {
                popup( _( "Patient is dead.  Please remove corpse to proceed.  Exiting." ) );
                return;
            } else if( cyborg.typeId() == "bot_broken_cyborg" || cyborg.typeId() == "corpse" ) {
                popup( _( "ERROR Bionic Level Assessement : FULL CYBORG.  Autodoc Mk. XI can't opperate.  Please move patient to appropriate facility.  Exiting." ) );
                return;
            }

            uilist cmenu;
            cmenu.text = _( "Autodoc Mk. XI.  Status: Online.  Please choose operation." );
            cmenu.addentry( 1, true, 'i', _( "Choose Compact Bionic Module to install." ) );
            cmenu.addentry( 2, true, 'u', _( "Choose installed bionic to uninstall." ) );
            cmenu.query();

            switch( cmenu.ret ) {
                case 1: {
                    popup( _( "ERROR NO SPACE AVAILABLE.  Operation impossible.  Exiting." ) );
                    break;
                }
                case 2: {
                    std::vector<std::string> choice_names;
                    choice_names.push_back( _( "Personality_Override" ) );
                    for( size_t i = 0; i < 6; i++ ) {
                        choice_names.push_back( _( "C0RR#PTED?D#TA" ) );
                    }
                    int choice_index = uilist( _( "Choose bionic to uninstall" ), choice_names );
                    if( choice_index == 0 ) {
                        g->save_cyborg( &cyborg, couch_pos, p );
                    } else {
                        popup( _( "UNKNOWN COMMAND.  Autodoc Mk. XI. Crashed." ) );
                        return;
                    }
                    break;
                }
                default:
                    return;
            }
            return;
        } else {
            popup( _( "No patient found located on the connected couches.  Operation impossible.  Exiting." ) );
            return;
        }
    } else if( patient.activity.id() == "ACT_OPERATION" ) {
        popup( _( "Operation underway.  Please wait until the end of the current procedure.  Estimated time remaining: %s." ),
               to_string( time_duration::from_turns( patient.activity.moves_left / 100 ) ) );
        p.add_msg_if_player( m_info, _( "The autodoc is working on %s." ), patient.disp_name() );
        return;
    }

    uilist amenu;
    amenu.text = _( "Autodoc Mk. XI.  Status: Online.  Please choose operation" );
    amenu.addentry( INSTALL_CBM, true, 'i', _( "Choose Compact Bionic Module to install" ) );
    amenu.addentry( UNINSTALL_CBM, true, 'u', _( "Choose installed bionic to uninstall" ) );
    amenu.addentry( BONESETTING, true, 's', _( "Splint broken limbs" ) );

    amenu.query();

    bool needs_anesthesia = true;
    // Legacy
    std::vector<item_comp> acomps;
    std::vector<tool_comp> anesth_kit;
    int drug_count = 0;

    if( patient.has_trait( trait_NOPAIN ) || patient.has_bionic( bionic_id( "bio_painkiller" ) ) ||
        amenu.ret > 1 ) {
        needs_anesthesia = false;
    } else {
        const inventory &crafting_inv = p.crafting_inventory();
        std::vector<const item *> a_filter = crafting_inv.items_with( []( const item & it ) {
            return it.has_quality( quality_id( "ANESTHESIA" ) );
        } );
        std::vector<const item *> b_filter = crafting_inv.items_with( []( const item & it ) {
            return it.has_flag( "ANESTHESIA" ); // legacy
        } );
        for( const item *anesthesia_item : a_filter ) {
            if( anesthesia_item->ammo_remaining() >= 1 ) {
                anesth_kit.push_back( tool_comp( anesthesia_item->typeId(), 1 ) );
                drug_count += anesthesia_item->ammo_remaining();
            }
        }
        for( const item *anesthesia_item : b_filter ) {
            acomps.push_back( item_comp( anesthesia_item->typeId(), 1 ) ); // legacy
        }
    }

    switch( amenu.ret ) {
        case INSTALL_CBM: {
            item_location bionic = game_menus::inv::install_bionic( p, patient );

            if( !bionic ) {
                return;
            }

            const itype *itemtype = bionic.get_item()->type;

            player &installer = best_installer( p, null_player, itemtype->bionic->difficulty );
            if( &installer == &null_player ) {
                return;
            }

            const int weight = units::to_kilogram( patient.bodyweight() ) / 10;
            const int surgery_duration = itemtype->bionic->difficulty * 2;
            const requirement_data req_anesth = *requirement_id( "anesthetic" ) *
                                                surgery_duration * weight;

            if( patient.can_install_bionics( ( *itemtype ), installer, true ) ) {
                const time_duration duration = itemtype->bionic->difficulty * 20_minutes;
                patient.introduce_into_anesthesia( duration, installer, needs_anesthesia );
                bionic.remove_item();
                if( needs_anesthesia ) {
                    // Consume obsolete anesthesia first
                    if( acomps.empty() ) {
                        for( const auto &e : req_anesth.get_components() ) {
                            p.consume_items( e, 1, is_crafting_component );
                        }
                        for( const auto &e : req_anesth.get_tools() ) {
                            p.consume_tools( e );
                        }
                        p.invalidate_crafting_inventory();
                    } else {
                        // Legacy
                        p.consume_items( acomps, 1, is_crafting_component );
                    }

                }
                installer.mod_moves( -to_moves<int>( 1_minutes ) );
                patient.install_bionics( ( *itemtype ), installer, true );
            }
            break;
        }

        case UNINSTALL_CBM: {
            bionic_collection installed_bionics = *patient.my_bionics;
            if( installed_bionics.empty() ) {
                //~ %1$s is patient name
                popup_player_or_npc( patient, _( "You don't have any bionics installed." ),
                                     _( "%1$s doesn't have any bionics installed." ) );
                return;
            }

            for( auto &bio : installed_bionics ) {
                if( bio.id != bionic_id( "bio_power_storage" ) ||
                    bio.id != bionic_id( "bio_power_storage_mkII" ) ) {
                    if( item::type_is_defined( bio.id.str() ) ) {// put cbm items in your inventory
                        item bionic_to_uninstall( bio.id.str(), calendar::turn );
                        bionic_to_uninstall.set_flag( "IN_CBM" );
                        bionic_to_uninstall.set_flag( "NO_STERILE" );
                        bionic_to_uninstall.set_flag( "NO_PACKED" );
                        g->u.i_add( bionic_to_uninstall );
                    }
                }
            }

            const item_location bionic = game_menus::inv::uninstall_bionic( p, patient );
            if( !bionic ) {
                g->u.remove_items_with( []( const item & it ) {// remove cbm items from inventory
                    return it.has_flag( "IN_CBM" );
                } );
                return;
            }
            const item *it = bionic.get_item();
            const itype *itemtype = it->type;
            const bionic_id &bid = itemtype->bionic->id;

            g->u.remove_items_with( []( const item & it ) {// remove cbm items from inventory
                return it.has_flag( "IN_CBM" );
            } );

            // Malfunctioning bionics that don't have associated items and get a difficulty of 12
            const int difficulty = itemtype->bionic ? itemtype->bionic->difficulty : 12;
            const float volume_anesth = difficulty * 20 * 2; // 2ml/min

            player &installer = best_installer( p, null_player, difficulty );
            if( &installer == &null_player ) {
                return;
            }

            if( patient.can_uninstall_bionic( bid, installer, true ) ) {
                const time_duration duration = difficulty * 20_minutes;
                patient.introduce_into_anesthesia( duration, installer, needs_anesthesia );
                if( needs_anesthesia ) {
                    if( acomps.empty() ) {
                        // consume obsolete anesthesia first
                        p.consume_tools( anesth_kit, volume_anesth );
                    } else {
                        p.consume_items( acomps, 1, is_crafting_component ); // legacy
                    }

                }
                installer.mod_moves( -to_moves<int>( 1_minutes ) );
                patient.uninstall_bionic( bid, installer, true );
            }
            break;
        }

        case BONESETTING: {
            int broken_limbs_count = 0;
            for( int i = 0; i < num_hp_parts; i++ ) {
                const bool broken = patient.get_hp( static_cast<hp_part>( i ) ) <= 0;
                body_part part = player::hp_to_bp( static_cast<hp_part>( i ) );
                effect &existing_effect = patient.get_effect( effect_mending, part );
                // Skip part if not broken or already healed 50%
                if( !broken || ( !existing_effect.is_null() &&
                                 existing_effect.get_duration() >
                                 existing_effect.get_max_duration() - 5_days - 1_turns ) ) {
                    continue;
                }
                broken_limbs_count++;
                patient.moves -= 500;
                patient.add_msg_player_or_npc( m_good, _( "The machine rapidly sets and splints your broken %s." ),
                                               _( "The machine rapidly sets and splints <npcname>'s broken %s." ),
                                               body_part_name( part ) );
                // TODO: fail here if unable to perform the action, i.e. can't wear more, trait mismatch.
                if( !patient.worn_with_flag( "SPLINT", part ) ) {
                    item splint;
                    if( i == hp_arm_l || i == hp_arm_r ) {
                        splint = item( "arm_splint", 0 );
                    } else if( i == hp_leg_l || i == hp_leg_r ) {
                        splint = item( "leg_splint", 0 );
                    }
                    item &equipped_splint = patient.i_add( splint );
                    cata::optional<std::list<item>::iterator> worn_item =
                        patient.wear( equipped_splint, false );
                    if( worn_item && !patient.worn_with_flag( "SPLINT", part ) ) {
                        patient.change_side( **worn_item, false );
                    }
                }
                patient.add_effect( effect_mending, 0_turns, part, true );
                effect &mending_effect = patient.get_effect( effect_mending, part );
                mending_effect.set_duration( mending_effect.get_max_duration() - 5_days );
            }
            if( broken_limbs_count == 0 ) {
                //~ %1$s is patient name
                popup_player_or_npc( patient, _( "You have no limbs that require splinting." ),
                                     _( "%1$s doesn't have limbs that require splinting." ) );
            }
            break;
        }

        default:
            return;
    }
}

namespace sm_rack
{
const int MIN_CHARCOAL = 100;
const int CHARCOAL_PER_LITER = 25;
const units::volume MAX_FOOD_VOLUME = units::from_liter( 20 );
const units::volume MAX_FOOD_VOLUME_PORTABLE = units::from_liter( 15 );
} // namespace sm_rack

static int get_charcoal_charges( units::volume food )
{
    const int charcoal = to_liter( food ) * sm_rack::CHARCOAL_PER_LITER;

    return  std::max( charcoal, sm_rack::MIN_CHARCOAL );
}

static bool is_non_rotten_crafting_component( const item &it )
{
    return is_crafting_component( it ) && !it.rotten();
}

static void mill_activate( player &p, const tripoint &examp )
{
    const furn_id cur_mill_type = g->m.furn( examp );
    furn_id next_mill_type = f_null;
    if( cur_mill_type == f_wind_mill ) {
        next_mill_type = f_wind_mill_active;
    } else if( cur_mill_type == f_water_mill ) {
        next_mill_type = f_water_mill_active;
    } else {
        debugmsg( "Examined furniture has action mill_activate, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }
    bool food_present = false;
    map_stack items = g->m.i_at( examp );
    units::volume food_volume = 0_ml;

    for( item &it : items ) {
        if( it.typeId() == "flour" ) {
            add_msg( _( "This mill already contains flour." ) );
            add_msg( _( "Remove it before starting the mill again." ) );
            return;
        }
        if( it.has_flag( "MILLABLE" ) ) {
            food_present = true;
            food_volume += it.volume();
            continue;
        }
        if( !it.has_flag( "MILLABLE" ) ) {
            add_msg( m_bad, _( "This rack contains %s, which can't be milled!" ), it.tname( 1,
                     false ) );
            add_msg( _( "You remove the %s from the mill." ), it.tname() );
            g->m.add_item_or_charges( p.pos(), it );
            g->m.i_rem( examp, &it );
            p.mod_moves( -p.item_handling_cost( it ) );
            return;
        }
    }
    if( !food_present ) {
        add_msg( _( "This mill is empty.  Fill it with starchy products such as wheat, barley or oats and try again." ) );
        return;
    }
    // TODO : currently mill just uses sm_rack defined max volume
    if( food_volume > sm_rack::MAX_FOOD_VOLUME ) {
        add_msg( _( "This mill is overloaded with products, and the millstone can't turn.  Remove some and try again." ) );
        add_msg( pgettext( "volume units", "You think that you can load about %s %s in it." ),
                 format_volume( sm_rack::MAX_FOOD_VOLUME ), volume_units_long() );
        return;
    }

    for( auto &it : g->m.i_at( examp ) ) {
        if( it.has_flag( "MILLABLE" ) ) {
            // Do one final rot check before milling, then apply the PROCESSING flag to prevent further checks.
            it.process_temperature_rot( 1, examp, nullptr );
            it.set_flag( "PROCESSING" );
        }
    }
    g->m.furn_set( examp, next_mill_type );
    item result( "fake_milling_item", calendar::turn );
    result.item_counter = to_turns<int>( 6_hours );
    result.activate();
    g->m.add_item( examp, result );
    add_msg( _( "You remove the brake on the millstone and it slowly starts to turn." ) );
}

static void smoker_activate( player &p, const tripoint &examp )
{
    furn_id cur_smoker_type = g->m.furn( examp );
    furn_id next_smoker_type = f_null;
    const bool portable = g->m.furn( examp ) == furn_str_id( "f_metal_smoking_rack" ) ||
                          g->m.furn( examp ) == furn_str_id( "f_metal_smoking_rack_active" );
    if( cur_smoker_type == f_smoking_rack ) {
        next_smoker_type = f_smoking_rack_active;
    } else if( cur_smoker_type == f_metal_smoking_rack ) {
        next_smoker_type = f_metal_smoking_rack_active;
    } else {
        debugmsg( "Examined furniture has action smoker_activate, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }
    bool food_present = false;
    bool charcoal_present = false;
    map_stack items = g->m.i_at( examp );
    units::volume food_volume = 0_ml;
    item *charcoal = nullptr;

    for( item &it : items ) {
        if( it.has_flag( "SMOKED" ) && !it.has_flag( "SMOKABLE" ) ) {
            add_msg( _( "This rack already contains smoked food." ) );
            add_msg( _( "Remove it before firing the smoking rack again." ) );
            return;
        }
        if( it.has_flag( "SMOKABLE" ) ) {
            food_present = true;
            food_volume += it.volume();
            continue;
        }
        if( it.typeId() == "charcoal" ) {
            charcoal_present = true;
            charcoal = &it;
        }
        if( it.typeId() != "charcoal" && !it.has_flag( "SMOKABLE" ) ) {
            add_msg( m_bad, _( "This rack contains %s, which can't be smoked!" ), it.tname( 1,
                     false ) );
            add_msg( _( "You remove %s from the rack." ), it.tname() );
            g->m.add_item_or_charges( p.pos(), it );
            p.mod_moves( -p.item_handling_cost( it ) );
            g->m.i_rem( examp, &it );
            return;
        }
        if( it.has_flag( "SMOKED" ) && it.has_flag( "SMOKABLE" ) ) {
            add_msg( _( "This rack has some smoked food that might be dehydrated by smoking it again." ) );
        }
    }
    if( !food_present ) {
        add_msg( _( "This rack is empty.  Fill it with raw meat, fish or sausages and try again." ) );
        return;
    }
    if( !charcoal_present ) {
        add_msg( _( "There is no charcoal in the rack." ) );
        return;
    }
    if( portable && food_volume > sm_rack::MAX_FOOD_VOLUME_PORTABLE ) {
        add_msg( _( "This rack is overloaded with food, and it blocks the flow of smoke.  Remove some and try again." ) );
        add_msg( _( "You think that you can load about %s %s in it." ),
                 format_volume( sm_rack::MAX_FOOD_VOLUME_PORTABLE ), volume_units_long() );
        return;
    } else if( food_volume > sm_rack::MAX_FOOD_VOLUME ) {
        add_msg( _( "This rack is overloaded with food, and it blocks the flow of smoke.  Remove some and try again." ) );
        add_msg( _( "You think that you can load about %s %s in it." ),
                 format_volume( sm_rack::MAX_FOOD_VOLUME ), volume_units_long() );
        return;
    }

    int char_charges = get_charcoal_charges( food_volume );

    if( count_charges_in_list( charcoal->type, g->m.i_at( examp ) ) < char_charges ) {
        add_msg( _( "There is not enough charcoal in the rack to smoke this much food." ) );
        add_msg( _( "You need at least %1$s pieces of charcoal, and the smoking rack has %2$s inside." ),
                 char_charges, count_charges_in_list( charcoal->type, g->m.i_at( examp ) ) );
        return;
    }

    if( !p.has_charges( "fire", 1 ) ) {
        add_msg( _( "This smoking rack is ready to be fired, but you have no fire source." ) );
        return;
    } else if( !query_yn( _( "Fire the smoking rack?" ) ) ) {
        return;
    }

    p.use_charges( "fire", 1 );
    for( auto &it : g->m.i_at( examp ) ) {
        if( it.has_flag( "SMOKABLE" ) ) {
            it.process_temperature_rot( 1, examp, nullptr );
            it.set_flag( "PROCESSING" );
        }
    }
    g->m.furn_set( examp, next_smoker_type );
    if( charcoal->charges == char_charges ) {
        g->m.i_rem( examp, charcoal );
    } else {
        charcoal->charges -= char_charges;
    }
    item result( "fake_smoke_plume", calendar::turn );
    result.item_counter = to_turns<int>( 6_hours );
    result.activate();
    g->m.add_item( examp, result );
    add_msg( _( "You light a small fire under the rack and it starts to smoke." ) );
}

void iexamine::mill_finalize( player &, const tripoint &examp, const time_point &start_time )
{
    const furn_id cur_mill_type = g->m.furn( examp );
    furn_id next_mill_type = f_null;
    if( cur_mill_type == f_wind_mill_active ) {
        next_mill_type = f_wind_mill;
    } else if( cur_mill_type == f_water_mill_active ) {
        next_mill_type = f_water_mill;
    } else {
        debugmsg( "Furniture executed action mill_finalize, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }

    map_stack items = g->m.i_at( examp );
    if( items.empty() ) {
        g->m.furn_set( examp, next_mill_type );
        return;
    }

    for( item &it : items ) {
        if( it.has_flag( "MILLABLE" ) && it.get_comestible() ) {
            it.calc_rot_while_processing( 6_hours );
            item result( "flour", start_time + 6_hours, it.charges * 15 );
            // Set flag to tell set_relative_rot() to calc from bday not now
            result.set_flag( "PROCESSING_RESULT" );
            result.set_relative_rot( it.get_relative_rot() );
            result.unset_flag( "PROCESSING_RESULT" );
            it = result;
        }
    }
    g->m.furn_set( examp, next_mill_type );
}

static void smoker_finalize( player &, const tripoint &examp, const time_point &start_time )
{
    furn_id cur_smoker_type = g->m.furn( examp );
    furn_id next_smoker_type = f_null;
    if( cur_smoker_type == f_smoking_rack_active ) {
        next_smoker_type = f_smoking_rack;
    } else if( cur_smoker_type == f_metal_smoking_rack_active ) {
        next_smoker_type = f_metal_smoking_rack;
    } else {
        debugmsg( "Furniture executed action smoker_finalize, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }

    map_stack items = g->m.i_at( examp );
    if( items.empty() ) {
        g->m.furn_set( examp, next_smoker_type );
        return;
    }

    for( item &it : items ) {
        if( it.has_flag( "SMOKABLE" ) && it.get_comestible() ) {
            if( it.get_comestible()->smoking_result.empty() ) {
                it.unset_flag( "PROCESSING" );
            } else {
                it.calc_rot_while_processing( 6_hours );

                item result( it.get_comestible()->smoking_result, start_time + 6_hours, it.charges );

                // Set flag to tell set_relative_rot() to calc from bday not now
                result.set_flag( "PROCESSING_RESULT" );
                result.set_relative_rot( it.get_relative_rot() );
                result.unset_flag( "PROCESSING_RESULT" );
                it = result;
            }
        }
    }
    g->m.furn_set( examp, next_smoker_type );
}

static void smoker_load_food( player &p, const tripoint &examp,
                              const units::volume &remaining_capacity )
{
    std::vector<item_comp> comps;

    if( g->m.furn( examp ) == furn_str_id( "f_smoking_rack_active" ) ||
        g->m.furn( examp ) == furn_str_id( "f_metal_smoking_rack_active" ) ) {
        p.add_msg_if_player( _( "You can't place more food while it's smoking." ) );
        return;
    }
    // filter SMOKABLE food
    inventory inv = p.crafting_inventory();
    inv.remove_items_with( []( const item & it ) {
        return it.rotten();
    } );
    std::vector<const item *> filtered = p.crafting_inventory().items_with( []( const item & it ) {
        return it.has_flag( "SMOKABLE" );
    } );

    uilist smenu;
    smenu.text = _( "Load smoking rack with what kind of food?" );
    // count and ask for item to be placed ...
    std::list<std::string> names;
    std::vector<const item *> entries;
    for( const item *smokable_item : filtered ) {
        int count;
        if( smokable_item->count_by_charges() ) {
            count = inv.charges_of( smokable_item->typeId() );
        } else {
            count = inv.amount_of( smokable_item->typeId() );
        }
        if( count != 0 ) {
            auto on_list = std::find( names.begin(), names.end(), item::nname( smokable_item->typeId(), 1 ) );
            if( on_list == names.end() ) {
                smenu.addentry( item::nname( smokable_item->typeId(), 1 ) );
                entries.push_back( smokable_item );
            }
            names.push_back( item::nname( smokable_item->typeId(), 1 ) );
            comps.push_back( item_comp( smokable_item->typeId(), count ) );
        }
    }

    if( comps.empty() ) {
        p.add_msg_if_player( _( "You don't have any food that can be smoked." ) );
        return;
    }

    smenu.query();

    if( smenu.ret < 0 || static_cast<size_t>( smenu.ret ) >= entries.size() ) {
        add_msg( m_info, _( "Never mind." ) );
        return;
    }
    int count = 0;
    auto what = entries[smenu.ret];
    for( const auto &c : comps ) {
        if( c.type == what->typeId() ) {
            count = c.count;
        }
    }

    const int max_count_for_capacity = remaining_capacity / what->base_volume();
    const int max_count = std::min( count, max_count_for_capacity );

    // ... then ask how many to put it
    const std::string popupmsg = string_format( _( "Insert how many %s into the rack?" ),
                                 item::nname( what->typeId(), count ) );
    int amount = string_input_popup()
                 .title( popupmsg )
                 .width( 20 )
                 .text( to_string( max_count ) )
                 .only_digits( true )
                 .query_int();

    if( amount == 0 ) {
        add_msg( m_info, _( "Never mind." ) );
        return;
    } else if( amount > count ) {
        add_msg( m_info, _( "You don't have that many." ) );
        return;
    } else if( amount > max_count_for_capacity ) {
        add_msg( m_info, _( "You can't place that many." ) );
        return;
    }

    // reload comps with chosen items and quantity
    comps.clear();
    comps.push_back( item_comp( what->typeId(), amount ) );

    // select from where to get the items from and place them
    inv.form_from_map( g->u.pos(), PICKUP_RANGE, &g->u );
    inv.remove_items_with( []( const item & it ) {
        return it.rotten();
    } );

    comp_selection<item_comp> selected = p.select_item_component( comps, 1, inv, true,
                                         is_non_rotten_crafting_component );
    std::list<item> moved = p.consume_items( selected, 1, is_non_rotten_crafting_component );

    for( const item &m : moved ) {
        g->m.add_item( examp, m );
        p.mod_moves( -p.item_handling_cost( m ) );
        add_msg( m_info, _( "You carefully place %s %s in the rack." ), amount,
                 item::nname( m.typeId(), amount ) );
    }
    p.invalidate_crafting_inventory();
}

static void mill_load_food( player &p, const tripoint &examp,
                            const units::volume &remaining_capacity )
{
    std::vector<item_comp> comps;

    if( g->m.furn( examp ) == furn_str_id( "f_wind_mill_active" ) ||
        g->m.furn( examp ) == furn_str_id( "f_water_mill_active" ) ) {
        p.add_msg_if_player( _( "You can't place more food while it's milling." ) );
        return;
    }
    // filter millable food
    inventory inv = p.crafting_inventory();
    inv.remove_items_with( []( const item & it ) {
        return it.rotten();
    } );
    std::vector<const item *> filtered = p.crafting_inventory().items_with( []( const item & it ) {
        return it.has_flag( "MILLABLE" );
    } );

    uilist smenu;
    smenu.text = _( "Load mill with what kind of product?" );
    // count and ask for item to be placed ...
    std::list<std::string> names;
    std::vector<const item *> entries;
    for( const item *millable_item : filtered ) {
        int count;
        if( millable_item->count_by_charges() ) {
            count = inv.charges_of( millable_item->typeId() );
        } else {
            count = inv.amount_of( millable_item->typeId() );
        }
        if( count != 0 ) {
            auto on_list = std::find( names.begin(), names.end(), item::nname( millable_item->typeId(), 1 ) );
            if( on_list == names.end() ) {
                smenu.addentry( item::nname( millable_item->typeId(), 1 ) );
                entries.push_back( millable_item );
            }
            names.push_back( item::nname( millable_item->typeId(), 1 ) );
            comps.push_back( item_comp( millable_item->typeId(), count ) );
        }
    }

    if( comps.empty() ) {
        p.add_msg_if_player( _( "You don't have any products that can be milled." ) );
        return;
    }

    smenu.query();

    if( smenu.ret < 0 || static_cast<size_t>( smenu.ret ) >= entries.size() ) {
        add_msg( m_info, _( "Never mind." ) );
        return;
    }
    int count = 0;
    auto what = entries[smenu.ret];
    for( const auto &c : comps ) {
        if( c.type == what->typeId() ) {
            count = c.count;
        }
    }

    const int max_count_for_capacity = remaining_capacity / what->base_volume();
    const int max_count = std::min( count, max_count_for_capacity );

    // ... then ask how many to put it
    const std::string popupmsg = string_format( _( "Insert how many %s into the mill?" ),
                                 item::nname( what->typeId(), count ) );
    int amount = string_input_popup()
                 .title( popupmsg )
                 .width( 20 )
                 .text( to_string( max_count ) )
                 .only_digits( true )
                 .query_int();

    if( amount == 0 ) {
        add_msg( m_info, _( "Never mind." ) );
        return;
    } else if( amount > count ) {
        add_msg( m_info, _( "You don't have that many." ) );
        return;
    } else if( amount > max_count_for_capacity ) {
        add_msg( m_info, _( "You can't place that many." ) );
        return;
    }

    // reload comps with chosen items and quantity
    comps.clear();
    comps.push_back( item_comp( what->typeId(), amount ) );

    // select from where to get the items from and place them
    inv.form_from_map( g->u.pos(), PICKUP_RANGE, &g->u );
    inv.remove_items_with( []( const item & it ) {
        return it.rotten();
    } );

    comp_selection<item_comp> selected = p.select_item_component( comps, 1, inv, true,
                                         is_non_rotten_crafting_component );
    std::list<item> moved = p.consume_items( selected, 1, is_non_rotten_crafting_component );
    for( const item &m : moved ) {
        g->m.add_item( examp, m );
        p.mod_moves( -p.item_handling_cost( m ) );
        add_msg( m_info, pgettext( "item amount and name", "You carefully place %s %s in the mill." ),
                 amount, item::nname( m.typeId(), amount ) );
    }
    p.invalidate_crafting_inventory();
}

void iexamine::on_smoke_out( const tripoint &examp, const time_point &start_time )
{
    if( g->m.furn( examp ) == furn_str_id( "f_smoking_rack_active" ) ||
        g->m.furn( examp ) == furn_str_id( "f_metal_smoking_rack_active" ) ) {
        smoker_finalize( g->u, examp, start_time );
    }
}

void iexamine::quern_examine( player &p, const tripoint &examp )
{
    if( g->m.furn( examp ) == furn_str_id( "f_water_mill" ) ) {
        if( !g->m.is_water_shallow_current( examp ) ) {
            add_msg( _( "The water mill needs to be over shallow flowing water to work." ) );
            return;
        }
    }
    if( g->m.furn( examp ) == furn_str_id( "f_wind_mill" ) ) {
        if( g->is_sheltered( examp ) ) {
            add_msg( _( "The wind mill needs to be outside in the wind to work." ) );
            return;
        }
    }

    const bool active = g->m.furn( examp ) == furn_str_id( "f_water_mill_active" ) ||
                        g->m.furn( examp ) == furn_str_id( "f_wind_mill_active" );
    map_stack items_here = g->m.i_at( examp );

    if( items_here.empty() && active ) {
        debugmsg( "active mill was empty!" );
        if( g->m.furn( examp ) == furn_str_id( "f_water_mill_active" ) ) {
            g->m.furn_set( examp, f_water_mill );
        } else if( g->m.furn( examp ) == furn_str_id( "f_wind_mill_active" ) ) {
            g->m.furn_set( examp, f_wind_mill );
        }
        return;
    }

    if( items_here.size() == 1 && items_here.begin()->typeId() == "fake_milling_item" ) {
        debugmsg( "f_mill_active was empty, and had fake_milling_item!" );
        if( g->m.furn( examp ) == furn_str_id( "f_water_mill_active" ) ) {
            g->m.furn_set( examp, f_water_mill );
        } else if( g->m.furn( examp ) == furn_str_id( "f_wind_mill_active" ) ) {
            g->m.furn_set( examp, f_wind_mill );
        }
        items_here.erase( items_here.begin() );
        return;
    }

    std::stringstream pop;
    time_duration time_left = 0_turns;
    int hours_left = 0;
    int minutes_left = 0;
    units::volume f_volume = 0_ml;
    bool f_check = false;

    for( const item &it : items_here ) {
        if( it.is_food() ) {
            f_check = true;
            f_volume += it.volume();
        }
        if( active && it.typeId() == "fake_milling_item" ) {
            time_left = time_duration::from_turns( it.item_counter );
            hours_left = to_hours<int>( time_left );
            minutes_left = to_minutes<int>( time_left ) + 1;
        }
    }

    const bool empty = f_volume == 0_ml;
    const bool full = f_volume >= sm_rack::MAX_FOOD_VOLUME;
    const auto remaining_capacity = sm_rack::MAX_FOOD_VOLUME - f_volume;

    uilist smenu;
    smenu.text = _( "What to do with the mill?" );
    smenu.desc_enabled = true;

    smenu.addentry( 0, true, 'i', _( "Inspect mill" ) );

    if( !active ) {
        smenu.addentry_desc( 1, !empty, 'r',
                             empty ?  _( "Remove brake and start milling... insert some products for milling first" ) :
                             _( "Remove brake and start milling" ),
                             _( "Remove brake and start milling, milling will take about 6 hours." ) );

        smenu.addentry_desc( 2, !full, 'p',
                             full ? _( "Insert products for milling... mill is full" ) :
                             string_format( _( "Insert products for milling... remaining capacity is %s %s" ),
                                            format_volume( remaining_capacity ), volume_units_abbr() ),
                             _( "Fill the mill with starchy products such as wheat, barley or oats." ) );

        if( f_check ) {
            smenu.addentry( 3, f_check, 'e', _( "Remove products from mill" ) );
        }

    } else {
        smenu.addentry_desc( 4, true, 'x',
                             _( "Apply brake to mill" ),
                             _( "Applying the brake will stop milling process." ) );
    }

    smenu.query();

    switch( smenu.ret ) {
        case 0: { //inspect mill
            if( active ) {
                pop << "<color_green>" << _( "There's a mill here.  It is turning and milling." ) << "</color>"
                    << "\n";
                if( time_left > 0_turns ) {
                    if( minutes_left > 60 ) {
                        pop << string_format( ngettext( "It will finish milling in about %d hour.",
                                                        "It will finish milling in about %d hours.",
                                                        hours_left ), hours_left ) << "\n \n ";
                    } else if( minutes_left > 30 ) {
                        pop << _( "It will finish milling in less than an hour." );
                    } else {
                        pop << string_format( _( "It should take about %d minutes to finish milling." ), minutes_left );
                    }
                }
            } else {
                pop << "<color_green>" << _( "There's a mill here." ) << "</color>" << "\n";
            }
            pop << "<color_green>" << _( "You inspect its contents and find: " ) << "</color>" << "\n \n ";
            if( items_here.empty() ) {
                pop << _( "... that it is empty." );
            } else {
                for( const item &it : items_here ) {
                    if( it.typeId() == "fake_milling_item" ) {
                        pop << "\n " << "<color_red>" << _( "You see some grains that are not yet milled to fine flour." )
                            << "</color>" <<
                            "\n ";
                        continue;
                    }
                    pop << "-> " << item::nname( it.typeId(), it.charges );
                    pop << " (" << std::to_string( it.charges ) << ") \n ";
                }
            }
            popup( pop.str(), PF_NONE );
            break;
        }
        case 1: //activate
            if( active ) {
                add_msg( _( "It is already milling." ) );
            } else {
                mill_activate( p, examp );
            }
            break;
        case 2: // load food
            mill_load_food( p, examp, remaining_capacity );
            break;
        case 3: // remove food
            for( map_stack::iterator it = items_here.begin(); it != items_here.end(); ) {
                if( it->is_food() ) {
                    // get handling cost before the item reference is invalidated
                    const int handling_cost = -p.item_handling_cost( *it );

                    add_msg( _( "You remove %s from the mill." ), it->tname() );
                    g->m.add_item_or_charges( p.pos(), *it );
                    it = items_here.erase( it );
                    p.mod_moves( handling_cost );
                } else {
                    ++it;
                }
            }
            if( active ) {
                if( g->m.furn( examp ) == furn_str_id( "f_water_mill_active" ) ) {
                    g->m.furn_set( examp, f_water_mill );
                } else if( g->m.furn( examp ) == furn_str_id( "f_wind_mill_active" ) ) {
                    g->m.furn_set( examp, f_wind_mill );
                }
                add_msg( m_info, _( "You stop the milling process." ) );
            }
            break;
        default:
            add_msg( m_info, _( "Never mind." ) );
            break;
        case 4:
            if( g->m.furn( examp ) == furn_str_id( "f_water_mill_active" ) ) {
                g->m.furn_set( examp, f_water_mill );
            } else if( g->m.furn( examp ) == furn_str_id( "f_wind_mill_active" ) ) {
                g->m.furn_set( examp, f_wind_mill );
            }
            add_msg( m_info, _( "You stop the milling process." ) );
            break;
    }
}

void iexamine::smoker_options( player &p, const tripoint &examp )
{
    const bool active = g->m.furn( examp ) == furn_str_id( "f_smoking_rack_active" ) ||
                        g->m.furn( examp ) == furn_str_id( "f_metal_smoking_rack_active" );
    const bool portable = g->m.furn( examp ) == furn_str_id( "f_metal_smoking_rack" ) ||
                          g->m.furn( examp ) == furn_str_id( "f_metal_smoking_rack_active" );
    map_stack items_here = g->m.i_at( examp );

    if( portable && items_here.empty() && active ) {
        debugmsg( "f_metal_smoking_rack_active was empty!" );
        g->m.furn_set( examp, f_metal_smoking_rack );
        return;
    } else if( items_here.empty() && active ) {
        debugmsg( "f_smoking_rack_active was empty!" );
        g->m.furn_set( examp, f_smoking_rack );
        return;
    }
    if( portable && items_here.size() == 1 && items_here.begin()->typeId() == "fake_smoke_plume" ) {
        debugmsg( "f_metal_smoking_rack_active was empty, and had fake_smoke_plume!" );
        g->m.furn_set( examp, f_metal_smoking_rack );
        items_here.erase( items_here.begin() );
        return;
    } else if( items_here.size() == 1 && items_here.begin()->typeId() == "fake_smoke_plume" ) {
        debugmsg( "f_smoking_rack_active was empty, and had fake_smoke_plume!" );
        g->m.furn_set( examp, f_smoking_rack );
        items_here.erase( items_here.begin() );
        return;
    }

    bool rem_f_opt = false;
    std::stringstream pop;
    time_duration time_left = 0_turns;
    int hours_left = 0;
    int minutes_left = 0;
    units::volume f_volume = 0_ml;
    bool f_check = false;

    for( const item &it : items_here ) {
        if( it.is_food() ) {
            f_check = true;
            f_volume += it.volume();
        }
        if( active && it.typeId() == "fake_smoke_plume" ) {
            time_left = time_duration::from_turns( it.item_counter );
            hours_left = to_hours<int>( time_left );
            minutes_left = to_minutes<int>( time_left ) + 1;
        }
    }

    const bool empty = f_volume == 0_ml;
    const bool full = f_volume >= sm_rack::MAX_FOOD_VOLUME;
    const bool full_portable = f_volume >= sm_rack::MAX_FOOD_VOLUME_PORTABLE;
    const auto remaining_capacity = sm_rack::MAX_FOOD_VOLUME - f_volume;
    const auto remaining_capacity_portable = sm_rack::MAX_FOOD_VOLUME_PORTABLE - f_volume;
    const auto has_coal_in_inventory = p.charges_of( "charcoal" ) > 0;
    const auto coal_charges = count_charges_in_list( item::find_type( "charcoal" ), items_here );
    const auto need_charges = get_charcoal_charges( f_volume );
    const bool has_coal = coal_charges > 0;
    const bool has_enough_coal = coal_charges >= need_charges;

    uilist smenu;
    smenu.text = _( "What to do with the smoking rack:" );
    smenu.desc_enabled = true;

    smenu.addentry( 0, true, 'i', _( "Inspect smoking rack" ) );

    if( !active ) {
        smenu.addentry_desc( 1, !empty && has_enough_coal, 'l',
                             empty ?  _( "Light up and smoke food... insert some food for smoking first" ) :
                             !has_enough_coal ? string_format(
                                 _( "Light up and smoke food... need extra %d charges of charcoal" ),
                                 need_charges - coal_charges ) :
                             _( "Light up and smoke food" ),
                             _( "Light up the smoking rack and start smoking. Smoking will take about 6 hours." ) );
        if( portable ) {
            smenu.addentry_desc( 2, !full_portable, 'f',
                                 full_portable ? _( "Insert food for smoking... smoking rack is full" ) :
                                 string_format( _( "Insert food for smoking... remaining capacity is %s %s" ),
                                                format_volume( remaining_capacity_portable ), volume_units_abbr() ),
                                 _( "Fill the smoking rack with raw meat, fish or sausages for smoking or fruit or vegetable or smoked meat for drying." ) );

            smenu.addentry_desc( 8, !active, 'z',
                                 active ? _( "You cannot disassemble this smoking rack while it is active!" ) :
                                 _( "Disassemble the smoking rack" ), "" );

        } else {
            smenu.addentry_desc( 2, !full, 'f',
                                 full ? _( "Insert food for smoking... smoking rack is full" ) :
                                 string_format( _( "Insert food for smoking... remaining capacity is %s %s" ),
                                                format_volume( remaining_capacity ), volume_units_abbr() ),
                                 _( "Fill the smoking rack with raw meat, fish or sausages for smoking or fruit or vegetable or smoked meat for drying." ) );
        }

        if( f_check ) {
            smenu.addentry( 4, f_check, 'e', _( "Remove food from smoking rack" ) );
        }

        smenu.addentry_desc( 3, has_coal_in_inventory, 'r',
                             !has_coal_in_inventory ? _( "Reload with charcoal... you don't have any" ) :
                             _( "Reload with charcoal" ),
                             string_format(
                                 _( "You need %d charges of charcoal for %s %s of food. Minimal amount of charcoal is %d charges." ),
                                 sm_rack::CHARCOAL_PER_LITER, format_volume( 1000_ml ), volume_units_long(),
                                 sm_rack::MIN_CHARCOAL ) );

    } else {
        smenu.addentry_desc( 7, true, 'x',
                             _( "Quench burning charcoal" ),
                             _( "Quenching will stop smoking process, but also destroy all used charcoal." ) );
    }

    if( has_coal ) {
        smenu.addentry( 5, true, 'c',
                        active ? string_format( _( "Rake out %d excess charges of charcoal from smoking rack" ),
                                                coal_charges ) :
                        string_format( _( "Remove %d charges of charcoal from smoking rack" ), coal_charges ) );
    }

    smenu.query();

    switch( smenu.ret ) {
        case 0: { //inspect smoking rack
            if( active ) {
                pop << "<color_green>" << _( "There's a smoking rack here.  It is lit and smoking." ) << "</color>"
                    << "\n";
                if( time_left > 0_turns ) {
                    if( minutes_left > 60 ) {
                        pop << string_format( ngettext( "It will finish smoking in about %d hour.",
                                                        "It will finish smoking in about %d hours.",
                                                        hours_left ), hours_left ) << "\n \n ";
                    } else if( minutes_left > 30 ) {
                        pop << _( "It will finish smoking in less than an hour." ) << "\n ";
                    } else {
                        pop << string_format( _( "It should take about %d minutes to finish smoking." ),
                                              minutes_left ) << "\n ";
                    }
                }
            } else {
                pop << "<color_green>" << _( "There's a smoking rack here." ) << "</color>" << "\n";
            }
            pop << "<color_green>" << _( "You inspect its contents and find: " ) << "</color>" << "\n \n ";
            if( items_here.empty() ) {
                pop << _( "... that it is empty." );
            } else {
                for( const item &it : items_here ) {
                    if( it.typeId() == "fake_smoke_plume" ) {
                        pop << "\n " << "<color_red>" << _( "You see some smoldering embers there." ) << "</color>" <<
                            "\n ";
                        continue;
                    }
                    pop << "-> " << item::nname( it.typeId(), it.charges );
                    pop << " (" << std::to_string( it.charges ) << ") \n ";
                }
            }
            popup( pop.str(), PF_NONE );
            break;
        }
        case 1: //activate
            if( active ) {
                add_msg( _( "It is already lit and smoking." ) );
            } else {
                smoker_activate( p, examp );
            }
            break;
        case 2: // load food
            if( portable ) {
                smoker_load_food( p, examp, remaining_capacity_portable );
            } else {
                smoker_load_food( p, examp, remaining_capacity );
            }
            break;
        case 3: // load charcoal
            reload_furniture( p, examp );
            break;
        case 4: // remove food
            rem_f_opt = true;
        /* fallthrough */
        case 5: { //remove charcoal
            for( map_stack::iterator it = items_here.begin(); it != items_here.end(); ) {
                if( ( rem_f_opt && it->is_food() ) || ( !rem_f_opt && ( it->typeId() == "charcoal" ) ) ) {
                    // get handling cost before the item reference is invalidated
                    const int handling_cost = -p.item_handling_cost( *it );

                    add_msg( _( "You remove %s from the rack." ), it->tname() );
                    g->m.add_item_or_charges( p.pos(), *it );
                    it = items_here.erase( it );
                    p.mod_moves( handling_cost );
                } else {
                    ++it;
                }
            }
            if( portable && active && rem_f_opt ) {
                g->m.furn_set( examp, f_metal_smoking_rack );
                add_msg( m_info, _( "You stop the smoking process." ) );
            } else if( active && rem_f_opt ) {
                g->m.furn_set( examp, f_smoking_rack );
                add_msg( m_info, _( "You stop the smoking process." ) );
            }
        }
        break;
        default:
            add_msg( m_info, _( "Never mind." ) );
            break;
        case 7:
            if( portable ) {
                g->m.furn_set( examp, f_metal_smoking_rack );
                add_msg( m_info, _( "You stop the smoking process." ) );
            } else {
                g->m.furn_set( examp, f_smoking_rack );
                add_msg( m_info, _( "You stop the smoking process." ) );
            }
            break;
        case 8:
            g->m.furn_set( examp, f_metal_smoking_rack );
            deployed_furniture( p, examp );
            break;
    }
}

void iexamine::open_safe( player &, const tripoint &examp )
{
    add_msg( m_info, _( "You open the unlocked safe. " ) );
    g->m.furn_set( examp, f_safe_o );
}

void iexamine::workbench( player &p, const tripoint &examp )
{
    workbench_internal( p, examp, cata::nullopt );
}

void iexamine::workbench_internal( player &p, const tripoint &examp,
                                   const cata::optional<vpart_reference> &part )
{
    std::vector<item_location> crafts;
    std::string name;
    bool is_undeployable = false;

    bool items_at_loc = false;

    if( part ) {
        name = part->part().name();
        auto items_at_part = part->vehicle().get_items( part->part_index() );

        for( item &it : items_at_part ) {
            if( it.is_craft() ) {
                crafts.emplace_back( item_location( vehicle_cursor( part->vehicle(), part->part_index() ), &it ) );
            }
        }
    } else {
        name = g->m.furn( examp ).obj().name();
        if( item::type_is_defined( g->m.furn( examp ).obj().deployed_item ) ) {
            is_undeployable = true;
        }

        auto items_at_furn = g->m.i_at( examp );
        items_at_loc = !items_at_furn.empty();

        for( item &it : items_at_furn ) {
            if( it.is_craft() ) {
                crafts.emplace_back( item_location( map_cursor( examp ), &it ) );
            }
        }
    }

    uilist amenu;

    enum option : int {
        start_craft = 0,
        repeat_craft,
        start_long_craft,
        work_on_craft,
        get_items,
        undeploy
    };

    amenu.text = string_format( pgettext( "furniture", "What to do at the %s?" ), name );
    amenu.addentry( start_craft,      true,            '1', _( "Craft items" ) );
    amenu.addentry( repeat_craft,     true,            '2', _( "Recraft last recipe" ) );
    amenu.addentry( start_long_craft, true,            '3', _( "Craft as long as possible" ) );
    amenu.addentry( work_on_craft,    !crafts.empty(), '4', _( "Work on craft" ) );
    if( !part ) {
        amenu.addentry( get_items,    items_at_loc,    'g', _( "Get items" ) );
    }
    if( is_undeployable ) {
        amenu.addentry( undeploy,     true,            't', _( "Take down the %s" ), name );
    }

    amenu.query();

    const option choice = static_cast<option>( amenu.ret );
    switch( choice ) {
        case start_craft: {
            if( p.has_active_mutation( trait_SHELL2 ) ) {
                p.add_msg_if_player( m_info, _( "You can't craft while you're in your shell." ) );
            } else {
                p.craft( examp );
            }
            break;
        }
        case repeat_craft: {
            if( p.has_active_mutation( trait_SHELL2 ) ) {
                p.add_msg_if_player( m_info, _( "You can't craft while you're in your shell." ) );
            } else {
                p.recraft( examp );
            }
            break;
        }
        case start_long_craft: {
            if( p.has_active_mutation( trait_SHELL2 ) ) {
                p.add_msg_if_player( m_info, _( "You can't craft while you're in your shell." ) );
            } else {
                p.long_craft( examp );
            }
            break;
        }
        case work_on_craft: {
            std::vector<std::string> item_names;
            for( item_location &it : crafts ) {
                if( it ) {
                    item_names.emplace_back( it.get_item()->tname() );
                }
            }
            uilist amenu2( _( "Which craft to work on?" ), item_names );

            if( amenu2.ret == UILIST_CANCEL ) {
                break;
            }

            item *selected_craft = crafts[amenu2.ret].get_item();

            if( !p.can_continue_craft( *selected_craft ) ) {
                break;
            }
            const recipe &rec = selected_craft->get_making();
            if( p.has_recipe( &rec, p.crafting_inventory(), p.get_crafting_helpers() ) == -1 ) {
                p.add_msg_player_or_npc(
                    _( "You don't know the recipe for the %s and can't continue crafting." ),
                    _( "<npcname> doesn't know the recipe for the %s and can't continue crafting." ),
                    rec.result_name() );
                break;
            }
            p.add_msg_player_or_npc(
                pgettext( "in progress craft", "You start working on the %s." ),
                pgettext( "in progress craft", "<npcname> starts working on the %s." ),
                selected_craft->tname() );
            p.assign_activity( activity_id( "ACT_CRAFT" ) );
            p.activity.targets.push_back( crafts[amenu2.ret] );
            p.activity.values.push_back( 0 ); // Not a long craft
            break;
        }
        case get_items: {
            Pickup::pick_up( examp, 0 );
            break;
        }
        case undeploy: {
            deployed_furniture( p, examp );
            break;
        }
    }
}

/**
 * Given then name of one of the above functions, returns the matching function
 * pointer. If no match is found, defaults to iexamine::none but prints out a
 * debug message as a warning.
 * @param function_name The name of the function to get.
 * @return A function pointer to the specified function.
 */
iexamine_function iexamine_function_from_string( const std::string &function_name )
{
    static const std::map<std::string, iexamine_function> function_map = {{
            { "none", &iexamine::none },
            { "deployed_furniture", &iexamine::deployed_furniture },
            { "cvdmachine", &iexamine::cvdmachine },
            { "nanofab", &iexamine::nanofab },
            { "gaspump", &iexamine::gaspump },
            { "atm", &iexamine::atm },
            { "vending", &iexamine::vending },
            { "toilet", &iexamine::toilet },
            { "elevator", &iexamine::elevator },
            { "controls_gate", &iexamine::controls_gate },
            { "cardreader", &iexamine::cardreader },
            { "cardreader_robofac", &iexamine::cardreader_robofac },
            { "cardreader_fp", &iexamine::cardreader_foodplace },
            { "intercom", &iexamine::intercom },
            { "rubble", &iexamine::rubble },
            { "chainfence", &iexamine::chainfence },
            { "bars", &iexamine::bars },
            { "portable_structure", &iexamine::portable_structure },
            { "pit", &iexamine::pit },
            { "pit_covered", &iexamine::pit_covered },
            { "slot_machine", &iexamine::slot_machine },
            { "safe", &iexamine::safe },
            { "bulletin_board", &iexamine::bulletin_board },
            { "fault", &iexamine::fault },
            { "pedestal_wyrm", &iexamine::pedestal_wyrm },
            { "pedestal_temple", &iexamine::pedestal_temple },
            { "door_peephole", &iexamine::door_peephole },
            { "fswitch", &iexamine::fswitch },
            { "flower_poppy", &iexamine::flower_poppy },
            { "flower_cactus", &iexamine::flower_cactus },
            { "fungus", &iexamine::fungus },
            { "flower_dahlia", &iexamine::flower_dahlia },
            { "flower_marloss", &iexamine::flower_marloss },
            { "egg_sackbw", &iexamine::egg_sackbw },
            { "egg_sackcs", &iexamine::egg_sackcs },
            { "egg_sackws", &iexamine::egg_sackws },
            { "dirtmound", &iexamine::dirtmound },
            { "aggie_plant", &iexamine::aggie_plant },
            { "fvat_empty", &iexamine::fvat_empty },
            { "fvat_full", &iexamine::fvat_full },
            { "keg", &iexamine::keg },
            { "harvest_furn_nectar", &iexamine::harvest_furn_nectar },
            { "harvest_furn", &iexamine::harvest_furn },
            { "harvest_ter_nectar", &iexamine::harvest_ter_nectar },
            { "harvest_ter", &iexamine::harvest_ter },
            { "harvested_plant", &iexamine::harvested_plant },
            { "shrub_marloss", &iexamine::shrub_marloss },
            { "translocator", &iexamine::translocator },
            { "tree_marloss", &iexamine::tree_marloss },
            { "tree_hickory", &iexamine::tree_hickory },
            { "tree_maple", &iexamine::tree_maple },
            { "tree_maple_tapped", &iexamine::tree_maple_tapped },
            { "shrub_wildveggies", &iexamine::shrub_wildveggies },
            { "recycle_compactor", &iexamine::recycle_compactor },
            { "trap", &iexamine::trap },
            { "water_source", &iexamine::water_source },
            { "clean_water_source", &iexamine::clean_water_source },
            { "reload_furniture", &iexamine::reload_furniture },
            { "curtains", &iexamine::curtains },
            { "sign", &iexamine::sign },
            { "pay_gas", &iexamine::pay_gas },
            { "gunsafe_ml", &iexamine::gunsafe_ml },
            { "gunsafe_el", &iexamine::gunsafe_el },
            { "locked_object", &iexamine::locked_object },
            { "kiln_empty", &iexamine::kiln_empty },
            { "kiln_full", &iexamine::kiln_full },
            { "arcfurnace_empty", &iexamine::arcfurnace_empty },
            { "arcfurnace_full", &iexamine::arcfurnace_full },
            { "autoclave_empty", &iexamine::autoclave_empty },
            { "autoclave_full", &iexamine::autoclave_full },
            { "fireplace", &iexamine::fireplace },
            { "ledge", &iexamine::ledge },
            { "autodoc", &iexamine::autodoc },
            { "quern_examine", &iexamine::quern_examine },
            { "smoker_options", &iexamine::smoker_options },
            { "open_safe", &iexamine::open_safe },
            { "workbench", &iexamine::workbench }
        }
    };

    auto iter = function_map.find( function_name );
    if( iter != function_map.end() ) {
        return iter->second;
    }

    //No match found
    debugmsg( "Could not find an iexamine function matching '%s'!", function_name );
    return &iexamine::none;
}

hack_result iexamine::hack_attempt( player &p )
{
    if( p.has_trait( trait_ILLITERATE ) ) {
        return HACK_UNABLE;
    }
    bool using_electrohack = p.has_charges( "electrohack", 25 ) &&
                             query_yn( _( "Use electrohack?" ) );
    bool using_fingerhack = !using_electrohack && p.has_bionic( bionic_id( "bio_fingerhack" ) ) &&
                            p.power_level > 24 && query_yn( _( "Use fingerhack?" ) );

    if( !( using_electrohack || using_fingerhack ) ) {
        return HACK_UNABLE;
    }

    p.moves -= to_moves<int>( 5_minutes );
    p.practice( skill_computer, 20 );
    ///\EFFECT_COMPUTER increases success chance of hacking card readers
    int player_computer_skill_level = p.get_skill_level( skill_computer );
    int success = rng( player_computer_skill_level / 4 - 2, player_computer_skill_level * 2 );
    success += rng( -3, 3 );
    if( using_fingerhack ) {
        p.charge_power( -25 );
        success++;
    }
    if( using_electrohack ) {
        p.use_charges( "electrohack", 25 );
        success++;
    }

    // odds go up with int>8, down with int<8
    // 4 int stat is worth 1 computer skill here
    ///\EFFECT_INT increases success chance of hacking card readers
    success += rng( 0, static_cast<int>( ( p.int_cur - 8 ) / 2 ) );

    if( success < 0 ) {
        add_msg( _( "You cause a short circuit!" ) );
        if( success <= -5 ) {
            if( using_electrohack ) {
                add_msg( m_bad, _( "Your electrohack is ruined!" ) );
                p.use_amount( "electrohack", 1 );
            } else {
                add_msg( m_bad, _( "Your power is drained!" ) );
                p.charge_power( -rng( 0, p.power_level ) );
            }
        }
        return HACK_FAIL;
    } else if( success < 6 ) {
        return HACK_NOTHING;
    } else {
        return HACK_SUCCESS;
    }
}

void iexamine::practice_survival_while_foraging( player *p )
{
    ///\EFFECT_INT Intelligence caps survival skill gains from foraging
    const int max_forage_skill = p->int_cur / 3 + 1;
    ///\EFFECT_SURVIVAL decreases survival skill gain from foraging (NEGATIVE)
    const int max_exp = 2 * ( max_forage_skill - p->get_skill_level( skill_survival ) );
    // Award experience for foraging attempt regardless of success
    p->practice( skill_survival, rng( 1, max_exp ), max_forage_skill );
}
