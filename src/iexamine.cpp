#include "iexamine.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "ammo.h"
#include "basecamp.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "compatibility.h" // needed for the workaround for the std::to_string bug in some compilers
#include "coordinate_conversions.h"
#include "craft_command.h"
#include "debug.h"
#include "event.h"
#include "fungal_effects.h"
#include "game.h"
#include "harvest.h"
#include "input.h"
#include "inventory.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "material.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "iuse_actor.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "player.h"
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
#include "vpart_position.h"
#include "weather.h"

const mtype_id mon_dark_wyrm( "mon_dark_wyrm" );
const mtype_id mon_fungal_blossom( "mon_fungal_blossom" );
const mtype_id mon_spider_web_s( "mon_spider_web_s" );
const mtype_id mon_spider_widow_giant_s( "mon_spider_widow_giant_s" );
const mtype_id mon_spider_cellar_giant_s( "mon_spider_cellar_giant_s" );
const mtype_id mon_turret( "mon_turret" );
const mtype_id mon_turret_rifle( "mon_turret_rifle" );

const skill_id skill_computer( "computer" );
const skill_id skill_fabrication( "fabrication" );
const skill_id skill_electronics( "electronics" );
const skill_id skill_firstaid( "firstaid" );
const skill_id skill_mechanics( "mechanics" );
const skill_id skill_cooking( "cooking" );
const skill_id skill_survival( "survival" );

const efftype_id effect_pkill2( "pkill2" );
const efftype_id effect_teleglow( "teleglow" );
const efftype_id effect_sleep( "sleep" );

static const trait_id trait_AMORPHOUS( "AMORPHOUS" );
static const trait_id trait_ARACHNID_ARMS_OK( "ARACHNID_ARMS_OK" );
static const trait_id trait_BADKNEES( "BADKNEES" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_INSECT_ARMS_OK( "INSECT_ARMS_OK" );
static const trait_id trait_M_DEFENDER( "M_DEFENDER" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_M_FERTILE( "M_FERTILE" );
static const trait_id trait_M_SPORES( "M_SPORES" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PARKOUR( "PARKOUR" );
static const trait_id trait_THRESH_MARLOSS( "THRESH_MARLOSS" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );
static const trait_id trait_BURROW( "BURROW" );

static void pick_plant( player &p, const tripoint &examp, const std::string &itemType,
                        ter_id new_ter,
                        bool seeds = false );

/**
 * Nothing player can interact with here.
 */
void iexamine::none( player &p, const tripoint &examp )
{
    ( void )p; //unused
    add_msg( _( "That is a %s." ), g->m.name( examp ).c_str() );
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

    if( !reqs.can_make_with_inventory( p.crafting_inventory() ) ) {
        popup( "%s", reqs.list_missing().c_str() );
        return;
    }

    // Consume materials
    for( const auto &e : reqs.get_components() ) {
        p.consume_items( e );
    }
    for( const auto &e : reqs.get_tools() ) {
        p.consume_tools( e );
    }
    p.invalidate_crafting_inventory();

    // Apply flag to item
    loc->item_tags.insert( "DIAMOND" );
    add_msg( m_good, _( "You apply a diamond coating to your %s" ), loc->type_name().c_str() );
    p.mod_moves( -1000 );
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

    if( !reqs.can_make_with_inventory( p.crafting_inventory() ) ) {
        popup( "%s", reqs.list_missing().c_str() );
        return;
    }

    // Consume materials
    for( const auto &e : reqs.get_components() ) {
        p.consume_items( e );
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
    if( !query_yn( _( "Use the %s?" ), g->m.tername( examp ).c_str() ) ) {
        none( p, examp );
        return;
    }

    auto items = g->m.i_at( examp );
    for( auto item_it = items.begin(); item_it != items.end(); ++item_it ) {
        if( item_it->made_of( LIQUID ) ) {
            ///\EFFECT_DEX decreases chance of spilling gas from a pump
            if( one_in( 10 + p.get_dex() ) ) {
                add_msg( m_bad, _( "You accidentally spill the %s." ), item_it->type_name().c_str() );
                static const auto max_spill_volume = units::from_liter( 1 );
                const long max_spill_charges = std::max( 1l, item_it->charges_per_volume( max_spill_volume ) );
                ///\EFFECT_DEX decreases amount of gas spilled from a pump
                const int qty = rng( 1l, max_spill_charges * 8.0 / std::max( 1, p.get_dex() ) );

                item spill = item_it->split( qty );
                if( spill.is_null() ) {
                    g->m.add_item_or_charges( p.pos(), *item_it );
                    items.erase( item_it );
                } else {
                    g->m.add_item_or_charges( p.pos(), spill );
                }

            } else {
                g->handle_liquid_from_ground( item_it, examp, 1 );
            }
            return;
        }
    }
    add_msg( m_info, _( "Out of order." ) );
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
                add_msg( m_info, _( "Your account now holds %s." ), format_money( u.cash ) );
            }

            u.moves -= 100;
        }

        //! Prompt for an integral value clamped to [0, max].
        static long prompt_for_amount( const char *const msg, const long max ) {
            const std::string formatted = string_format( msg, max );
            const long amount = string_input_popup()
                                .title( formatted )
                                .width( 20 )
                                .text( to_string( max ) )
                                .only_digits( true )
                                .query_long();

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
            u.moves -= 100;
            finish_interaction();

            return true;
        }

        //!Deposit money from cash card into bank account.
        bool do_deposit_money() {
            long money = u.charges_of( "cash_card" );

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
            u.moves -= 100;
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
            u.moves -= 100;
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
                i->charges =  0;
                u.moves    -= 10;
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
    constexpr int moves_cost = 250;
    long money = p.charges_of( "cash_card" );
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

    catacurses::window const w = catacurses::newwin( window_h, w_items_w, padding_y, padding_x );
    catacurses::window const w_item_info = catacurses::newwin( window_h, w_info_w,  padding_y,
                                           padding_x + w_items_w );

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
        mvwhline( w, first_item_offset - 1, 1, LINE_OXOX, w_items_w - 2 );
        mvwaddch( w, first_item_offset - 1, 0, LINE_XXXO ); // |-
        mvwaddch( w, first_item_offset - 1, w_items_w - 1, LINE_XOXX ); // -|

        trim_and_print( w, 1, 2, w_items_w - 3, c_light_gray,
                        _( "Money left: %s" ), format_money( money ) );

        // Keep the item selector centered in the page.
        int page_beg = 0;
        int page_end = page_size;
        if( cur_pos < num_items - cur_pos ) {
            page_beg = std::max( 0, cur_pos - lines_above );
            page_end = std::min( num_items, page_beg + list_lines );
        } else {
            page_end = std::min( num_items, cur_pos + lines_below );
            page_beg = std::max( 0, page_end - list_lines );
        }

        for( int line = 0; line < page_size; ++line ) {
            const int i = page_beg + line;
            const auto color = ( i == cur_pos ) ? h_light_gray : c_light_gray;
            const auto &elem = item_list[i];
            const int count = elem->second.size();
            const char c = ( count < 10 ) ? ( '0' + count ) : '*';
            trim_and_print( w, first_item_offset + line, 1, w_items_w - 3, color, "%c %s", c,
                            elem->first.c_str() );
        }

        draw_scrollbar( w, cur_pos, list_lines, num_items, first_item_offset );
        wrefresh( w );

        // Item info
        auto &cur_items = item_list[static_cast<size_t>( cur_pos )]->second;
        auto &cur_item  = cur_items.back();

        werase( w_item_info );
        // | {line}|
        // 12      3
        fold_and_print( w_item_info, 1, 2, w_info_w - 3, c_light_gray, cur_item->info( true ) );
        wborder( w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

        //+<{name}>+
        //12      34
        const std::string name = utf8_truncate( cur_item->display_name(),
                                                static_cast<size_t>( w_info_w - 4 ) );
        mvwprintw( w_item_info, 0, 1, "<%s>", name.c_str() );
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

        ( void ) p; // @todo: use me
        g->handle_liquid_from_ground( water, examp );
    }
}

/**
 * If underground, move 2 levels up else move 2 levels down. Stable movement between levels 0 and -2.
 */
void iexamine::elevator( player &p, const tripoint &examp )
{
    ( void )p; //unused
    if( !query_yn( _( "Use the %s?" ), g->m.tername( examp ).c_str() ) ) {
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
    if( !query_yn( _( "Use the %s?" ), g->m.tername( examp ).c_str() ) ) {
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
                           "id_military" );
    if( p.has_amount( card_type, 1 ) && query_yn( _( "Swipe your ID card?" ) ) ) {
        p.mod_moves( -100 );
        for( const tripoint &tmp : g->m.points_in_radius( examp, 3 ) ) {
            if( g->m.ter( tmp ) == t_door_metal_locked ) {
                g->m.ter_set( tmp, t_floor );
                open = true;
            }
        }
        for( monster &critter : g->all_monsters() ) {
            // Check 1) same overmap coords, 2) turret, 3) hostile
            if( ms_to_omt_copy( g->m.getabs( critter.pos() ) ) == ms_to_omt_copy( g->m.getabs( examp ) ) &&
                ( critter.type->id == mon_turret ||
                  critter.type->id == mon_turret_rifle ) &&
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
        switch( hack_attempt( p ) ) {
            case HACK_FAIL:
                g->m.ter_set( examp, t_card_reader_broken );
                break;
            case HACK_NOTHING:
                add_msg( _( "Nothing happens." ) );
                break;
            case HACK_SUCCESS: {
                add_msg( _( "You activate the panel!" ) );
                add_msg( m_good, _( "The nearby doors slide into the floor." ) );
                g->m.ter_set( examp, t_card_reader_broken );
                for( const tripoint &tmp : g->m.points_in_radius( examp, 3 ) ) {
                    if( g->m.ter( tmp ) == t_door_metal_locked ) {
                        g->m.ter_set( tmp, t_floor );
                    }
                }
            }
            break;
            case HACK_UNABLE:
                add_msg(
                    m_info,
                    p.get_skill_level( skill_computer ) > 0 ?
                    _( "Looks like you need a %s, or a tool to hack it with." ) :
                    _( "Looks like you need a %s." ),
                    item::nname( card_type ).c_str()
                );
                break;
        }
    }
}

/**
 * Prompt removal of rubble. Select best shovel and invoke "CLEAR_RUBBLE" on tile.
 */
void iexamine::rubble( player &p, const tripoint &examp )
{
    int moves;
    if( p.has_quality( quality_id( "DIG" ), 3 ) || p.has_trait( trait_BURROW ) ) {
        moves = 1250;
    } else if( p.has_quality( quality_id( "DIG" ), 2 ) ) {
        moves = 2500;
    } else {
        add_msg( m_info, _( "If only you had a shovel..." ) );
        return;
    }
    if( ( g->m.veh_at( examp ) || !g->m.tr_at( examp ).is_null() ||
          g->critter_at( examp ) != nullptr ) &&
        !query_yn( _( "Clear up that %s?" ), g->m.furnname( examp ).c_str() ) ) {
        return;
    }
    p.assign_activity( activity_id( "ACT_CLEAR_RUBBLE" ), moves, -1, 0 );
    p.activity.placement = examp;
    return;
}

/**
 * Try to pry crate open with crowbar.
 */
void iexamine::crate( player &p, const tripoint &examp )
{
    // PRY 1 is sufficient for popping open a nailed-shut crate.
    const bool has_prying_tool = p.crafting_inventory().has_quality( quality_id( "PRY" ), 1 );

    if( !has_prying_tool ) {
        add_msg( m_info, _( "If only you had something to pry with..." ) );
        return;
    }

    auto prying_items = p.crafting_inventory().items_with( []( const item & it ) -> bool {
        item temporary_item( it.type );
        return temporary_item.has_quality( quality_id( "PRY" ), 1 );
    } );

    iuse dummy;

    if( prying_items.size() == 1 ) {
        item temporary_item( prying_items[0]->type );
        // They only had one item anyway, so just use it.
        dummy.crowbar( &p, &temporary_item, false, examp );
        return;
    }

    // Sort by their quality level.
    std::sort( prying_items.begin(), prying_items.end(), []( const item * a, const item * b ) -> bool {
        return a->get_quality( quality_id( "PRY" ) ) > b->get_quality( quality_id( "PRY" ) );
    } );

    // Then display the items
    uilist selection_menu;
    selection_menu.text = string_format( _( "The %s is closed tightly." ),
                                         g->m.furnname( examp ) );

    int i = 0;
    selection_menu.addentry( i++, true, MENU_AUTOASSIGN, _( "Leave it alone" ) );
    for( auto iter : prying_items ) {
        selection_menu.addentry( i++, true, MENU_AUTOASSIGN, _( "Use your %s" ), iter->tname() );
    }

    selection_menu.selected = 1;
    selection_menu.query();
    auto index = selection_menu.ret;

    if( index == 0 || index == UILIST_CANCEL ) {
        none( p, examp );
        return;
    }

    auto selected_tool = prying_items[index - 1];
    item temporary_item( selected_tool->type );

    // if crowbar() ever eats charges or otherwise alters the passed item, rewrite this to reflect
    // changes to the original item.
    dummy.crowbar( &p, &temporary_item, false, examp );
}

/**
 * Prompt climbing over fence. Calculates move cost, applies it to player and, moves them.
 */
void iexamine::chainfence( player &p, const tripoint &examp )
{
    // Skip prompt if easy to climb.
    if( !g->m.has_flag( "CLIMB_SIMPLE", examp ) ) {
        if( !query_yn( _( "Climb %s?" ), g->m.tername( examp ).c_str() ) ) {
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
                 g->m.tername( examp ).c_str() );
        return;
    }
    if( !query_yn( _( "Slip through the %s?" ), g->m.tername( examp ).c_str() ) ) {
        none( p, examp );
        return;
    }
    p.moves -= 200;
    add_msg( _( "You slide right between the bars." ) );
    p.setpos( examp );
}

void iexamine::deployed_furniture( player &p, const tripoint &pos )
{
    if( !query_yn( _( "Take down the %s?" ), g->m.furn( pos ).obj().name().c_str() ) ) {
        return;
    }
    p.add_msg_if_player( m_info, _( "You take down the %s." ),
                         g->m.furn( pos ).obj().name().c_str() );
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

    p.moves -= 200;
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
        p.consume_items( planks );
        if( g->m.ter( examp ) == t_pit ) {
            g->m.ter_set( examp, t_pit_covered );
        } else if( g->m.ter( examp ) == t_pit_spiked ) {
            g->m.ter_set( examp, t_pit_spiked_covered );
        } else if( g->m.ter( examp ) == t_pit_glass ) {
            g->m.ter_set( examp, t_pit_glass_covered );
        }
        add_msg( _( "You place a plank of wood over the pit." ) );
        p.mod_moves( -100 );
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
    p.mod_moves( -100 );
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
    if( !( p.has_amount( "stethoscope", 1 ) || p.has_bionic( bionic_id( "bio_ears" ) ) ) ) {
        p.moves -= 100;
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

    if( query_yn( _( "Attempt to crack the safe?" ) ) ) {
        if( p.is_deaf() ) {
            add_msg( m_info, _( "You can't crack a safe while deaf!" ) );
            return;
        }
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
    p.moves -= ( 1000 - ( pick_quality * 100 ) ) - ( p.dex_cur + p.get_skill_level(
                   skill_mechanics ) ) * 5;
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
    switch( hack_attempt( p ) ) {
        case HACK_FAIL:
            p.add_memorial_log( pgettext( "memorial_male", "Set off an alarm." ),
                                pgettext( "memorial_female", "Set off an alarm." ) );
            sounds::sound( p.pos(), 60, sounds::sound_t::music, _( "an alarm sound!" ) );
            if( examp.z > 0 && !g->events.queued( EVENT_WANTED ) ) {
                g->events.add( EVENT_WANTED, calendar::turn + 30_minutes, 0, p.global_sm_location() );
            }
            break;
        case HACK_NOTHING:
            add_msg( _( "Nothing happens." ) );
            break;
        case HACK_SUCCESS:
            add_msg( _( "You successfully hack the gun safe." ) );
            g->m.furn_set( examp, furn_str_id( "f_safe_o" ) );
            break;
        case HACK_UNABLE:
            add_msg(
                m_info,
                p.get_skill_level( skill_computer ) > 0 ?
                _( "You can't hack this gun safe without a hacking tool." ) :
                _( "This electronic safe looks too complicated to open." )
            );
            break;
    }
}

/**
 * Checks PC has a crowbar then calls iuse.crowbar.
 */
void iexamine::locked_object( player &p, const tripoint &examp )
{
    const bool has_prying_tool = p.crafting_inventory().has_quality( quality_id( "PRY" ), 2 );
    if( !has_prying_tool ) {
        add_msg( m_info, _( "If only you had something to pry with..." ) );
        return;
    }

    auto prying_items = p.crafting_inventory().items_with( []( const item & it ) -> bool {
        item temporary_item( it.type );
        return temporary_item.has_quality( quality_id( "PRY" ), 2 );
    } );

    iuse dummy;
    if( prying_items.size() == 1 ) {
        item temporary_item( prying_items[0]->type );
        // They only had one item anyway, so just use it.
        dummy.crowbar( &p, &temporary_item, false, examp );
        return;
    }

    // Sort by their quality level.
    std::sort( prying_items.begin(), prying_items.end(), []( const item * a, const item * b ) -> int {
        return a->get_quality( quality_id( "PRY" ) ) > b->get_quality( quality_id( "PRY" ) );
    } );

    // Then display the items
    uilist selection_menu;
    selection_menu.text = string_format( _( "The %s is locked..." ), g->m.tername( examp ) );

    int i = 0;
    selection_menu.addentry( i++, true, MENU_AUTOASSIGN, _( "Leave it alone" ) );
    for( auto iter : prying_items ) {
        selection_menu.addentry( i++, true, MENU_AUTOASSIGN, string_format( _( "Use the %s" ),
                                 iter->tname() ) );
    }

    selection_menu.selected = 1;
    selection_menu.query();
    auto index = selection_menu.ret;

    if( index == 0 || index == UILIST_CANCEL ) {
        none( p, examp );
        return;
    }

    item temporary_item( prying_items[index - 1]->type );
    dummy.crowbar( &p, &temporary_item, false, examp );
}

void iexamine::bulletin_board( player &, const tripoint &examp )
{
    basecamp *camp = g->m.camp_at( examp );
    if( camp && camp->board_x() == examp.x && camp->board_y() == examp.y ) {
        std::vector<std::string> options;
        options.push_back( _( "Cancel" ) );
        // Causes a warning due to being unused, but don't want to delete
        // since it's clearly what's intended for future functionality.
        int choice = uilist( camp->board_name(), options );
        static_cast<void>( choice );
    } else {
        bool create_camp = g->m.allow_camp( examp );
        std::vector<std::string> options;
        if( create_camp ) {
            options.push_back( _( "Create camp" ) );
        }
        // @todo: Other Bulletin Boards
        int choice = uilist( _( "Bulletin Board" ), options );
        if( choice >= 0 && size_t( choice ) < options.size() ) {
            if( options[choice] == _( "Create camp" ) ) {
                // @todo: Allow text entry for name
                g->m.add_camp( examp, _( "Home" ) );
            }
        }
    }
}

/**
 * Display popup with reference to "The Enigma of Amigara Fault."
 */
void iexamine::fault( player &, const tripoint & )
{
    popup( _( "\
This wall is perfectly vertical.  Odd, twisted holes are set in it, leading\n\
as far back into the solid rock as you can see.  The holes are humanoid in\n\
shape, but with long, twisted, distended limbs." ) );
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
    g->u.add_memorial_log( pgettext( "memorial_male", "Awoke a group of dark wyrms!" ),
                           pgettext( "memorial_female", "Awoke a group of dark wyrms!" ) );
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
    sounds::sound( examp, 80, sounds::sound_t::combat, _( "an ominous grinding noise..." ) );
    g->m.ter_set( examp, t_rock_floor );
    g->events.add( EVENT_SPAWN_WYRMS, calendar::turn + rng( 5_turns, 10_turns ) );
}

/**
 * Put petrified eye on pedestal causing it to sink into ground and open temple.
 */
void iexamine::pedestal_temple( player &p, const tripoint &examp )
{

    if( g->m.i_at( examp ).size() == 1 &&
        g->m.i_at( examp )[0].typeId() == "petrified_eye" ) {
        add_msg( _( "The pedestal sinks into the ground..." ) );
        g->m.ter_set( examp, t_dirt );
        g->m.i_clear( examp );
        g->events.add( EVENT_TEMPLE_OPEN, calendar::turn + 4_turns );
    } else if( p.has_amount( "petrified_eye", 1 ) &&
               query_yn( _( "Place your petrified eye on the pedestal?" ) ) ) {
        p.use_amount( "petrified_eye", 1 );
        add_msg( _( "The pedestal sinks into the ground..." ) );
        g->m.ter_set( examp, t_dirt );
        g->events.add( EVENT_TEMPLE_OPEN, calendar::turn + 4_turns );
    } else
        add_msg( _( "This pedestal is engraved in eye-shaped diagrams, and has a \
large semi-spherical indentation at the top." ) );
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
    if( !query_yn( _( "Flip the %s?" ), g->m.tername( examp ).c_str() ) ) {
        none( p, examp );
        return;
    }
    ter_id terid = g->m.ter( examp );
    p.moves -= 100;
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
    g->events.add( EVENT_TEMPLE_SPAWN, calendar::turn + 3_turns );
}

/**
 * If it's winter: show msg and return true. Otherwise return false
 */
bool dead_plant( bool flower, player &p, const tripoint &examp )
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
bool can_drink_nectar( const player &p )
{
    return ( p.has_active_mutation( trait_id( "PROBOSCIS" ) )  ||
             p.has_active_mutation( trait_id( "BEAK_HUM" ) ) ) &&
           ( ( p.get_hunger() ) > 0 ) && ( !( p.wearing_something_on( bp_mouth ) ) );
}

/**
 * Consume Nectar. -15 hunger.
 */
bool drink_nectar( player &p )
{
    if( can_drink_nectar( p ) ) {
        p.moves -= 50; // Takes 30 seconds
        add_msg( _( "You drink some nectar." ) );
        p.mod_hunger( -15 );
        return true;
    }

    return false;
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
    // @todo: Get rid of this section and move it to eating
    // Two y/n prompts is just too much
    if( can_drink_nectar( p ) ) {
        if( !query_yn( _( "You feel woozy as you explore the %s. Drink?" ),
                       g->m.furnname( examp ).c_str() ) ) {
            return;
        }
        p.moves -= 150; // You take your time...
        add_msg( _( "You slowly suck up the nectar." ) );
        p.mod_hunger( -25 );
        p.mod_fatigue( 20 );
        p.add_effect( effect_pkill2, 7_minutes );
        // Please drink poppy nectar responsibly.
        if( one_in( 20 ) ) {
            p.add_addiction( ADD_PKILLER, 1 );
        }
    }
    if( !query_yn( _( "Pick %s?" ), g->m.furnname( examp ).c_str() ) ) {
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

    if( p.can_pickWeight( item( "poppy_bud" ), true ) &&
        p.can_pickVolume( item( "poppy_bud" ), true ) ) {
        p.i_add( item( "poppy_bud" ) );
        p.add_msg_if_player( _( "You harvest: poppy bud" ) );
    } else {
        g->m.add_item_or_charges( p.pos(), item( "poppy_bud" ) );
        p.add_msg_if_player( _( "You harvest and drop: poppy bud" ) );
    }
}

/**
 * It's a flower, drink nectar if your able to.
 */
void iexamine::flower_bluebell( player &p, const tripoint &examp )
{
    if( dead_plant( true, p, examp ) ) {
        return;
    }

    drink_nectar( p );

    // There was a bud and flower spawn here
    // But those were useless, don't re-add until they get useful
    none( p, examp );
    return;
}

/**
 * It's a flower, drink nectar if your able to.
 */
void iexamine::flower_tulip( player &p, const tripoint &examp )
{
    if( dead_plant( true, p, examp ) ) {
        return;
    }

    drink_nectar( p );

    // Add flower spawn once flowers are useful.
    none( p, examp );
    return;
}

/**
 * It's a flower, drink nectar if your able to.
 */
void iexamine::flower_spurge( player &p, const tripoint &examp )
{
    if( dead_plant( true, p, examp ) ) {
        return;
    }

    drink_nectar( p );

    // Add flower spawn once flowers are useful.
    none( p, examp );
    return;
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

    if( !p.has_quality( quality_id( "DIG" ) ) ) {
        none( p, examp );
        add_msg( m_info, _( "If only you had a shovel to dig up those roots..." ) );
        return;
    }

    if( !query_yn( _( "Pick %s?" ), g->m.furnname( examp ).c_str() ) ) {
        none( p, examp );
        return;
    }

    g->m.furn_set( examp, f_null );

    if( p.can_pickWeight( item( "dahlia_root" ), true ) &&
        p.can_pickVolume( item( "dahlia_root" ), true ) ) {
        p.i_add( item( "dahlia_root" ) );
        p.add_msg_if_player( _( "You harvest: dahlia root" ) );
    } else {
        g->m.add_item_or_charges( p.pos(), item( "dahlia_root" ) );
        p.add_msg_if_player( _( "You harvest and drop: dahlia root" ) );
    }
    // There was a bud and flower spawn here
    // But those were useless, don't re-add until they get useful
}

static bool harvest_common( player &p, const tripoint &examp, bool furn, bool nectar,
                            bool auto_forage = false )
{
    const auto hid = g->m.get_harvest( examp );
    if( hid.is_null() || hid->empty() ) {
        if( !auto_forage ) {
            p.add_msg_if_player( m_info, _( "Nothing can be harvested from this plant in current season" ) );
            iexamine::none( p, examp );
        }
        return false;
    }

    const auto &harvest = hid.obj();

    // If nothing can be harvested, neither can nectar
    // Incredibly low priority @todo: Allow separating nectar seasons
    if( nectar && drink_nectar( p ) ) {
        return false;
    }

    if( p.is_player() && !auto_forage &&
        !query_yn( _( "Pick %s?" ), furn ? g->m.furnname( examp ).c_str() : g->m.tername(
                       examp ).c_str() ) ) {
        iexamine::none( p, examp );
        return false;
    }

    int lev = p.get_skill_level( skill_survival );
    bool got_anything = false;
    for( const auto &entry : harvest ) {
        float min_num = entry.base_num.first + lev * entry.scale_num.first;
        float max_num = entry.base_num.second + lev * entry.scale_num.second;
        int roll = std::min<int>( entry.max, round( rng_float( min_num, max_num ) ) );
        for( int i = 0; i < roll; i++ ) {
            if( p.can_pickWeight( item( entry.drop ), true ) && p.can_pickVolume( item( entry.drop ), true ) ) {
                p.i_add( item( entry.drop ) );
                p.add_msg_if_player( _( "You harvest: %s" ), item( entry.drop ).tname().c_str() );
            } else {
                g->m.add_item_or_charges( p.pos(), item( entry.drop ) );
                p.add_msg_if_player( _( "You harvest and drop: %s" ), item( entry.drop ).tname().c_str() );
            }
            got_anything = true;
        }
    }

    if( !got_anything ) {
        p.add_msg_if_player( m_bad, _( "You couldn't harvest anything." ) );
    }

    p.mod_moves( -rng( 100, 300 ) );
    return true;
}

void iexamine::harvest_furn_nectar( player &p, const tripoint &examp )
{
    if( harvest_common( p, examp, true, true ) ) {
        g->m.furn_set( examp, f_null );
    }
}

void iexamine::harvest_furn( player &p, const tripoint &examp )
{
    if( harvest_common( p, examp, true, false ) ) {
        g->m.furn_set( examp, f_null );
    }
}

void iexamine::harvest_ter_nectar( player &p, const tripoint &examp )
{
    if( harvest_common( p, examp, false, true ) ) {
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
                       g->m.furnname( examp ).c_str() ) ) {
            return;
        }
        p.moves -= 50; // Takes 30 seconds
        add_msg( m_bad, _( "This flower tastes very wrong..." ) );
        // If you can drink flowers, you're post-thresh and the Mycus does not want you.
        p.add_effect( effect_teleglow, 10_minutes );
    }
    if( !query_yn( _( "Pick %s?" ), g->m.furnname( examp ).c_str() ) ) {
        none( p, examp );
        return;
    }
    g->m.furn_set( examp, f_null );
    g->m.spawn_item( p.pos(), "marloss_seed", 1, 3, calendar::turn );
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
    if( !query_yn( _( "Harvest the %s?" ), old_furn_name.c_str() ) ) {
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
    int roll = round( rng_float( 1, 5 ) );
    for( int i = 0; i < roll; i++ ) {
        if( monster_count == 0 && p.can_pickWeight( item( "spider_egg" ), true ) &&
            p.can_pickVolume( item( "spider_egg" ), true ) ) {
            p.i_add( item( "spider_egg" ) );
            p.add_msg_if_player( _( "You harvest: spider egg" ) );
        } else {
            g->m.add_item_or_charges( p.pos(), item( "spider_egg" ) );
            p.add_msg_if_player( _( "You harvest and drop: spider egg" ) );
        }
    }
    if( monster_count == 1 ) {
        add_msg( m_warning, _( "A spiderling bursts from the %s!" ), old_furn_name.c_str() );
    } else if( monster_count >= 1 ) {
        add_msg( m_warning, _( "Spiderlings burst from the %s!" ), old_furn_name.c_str() );
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
    add_msg( _( "The %s crumbles into spores!" ), g->m.furnname( examp ).c_str() );
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
                        seed_name.c_str(), seed_count );
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
    used_seed.front().set_age( 0_turns );
    g->m.add_item_or_charges( examp, used_seed.front() );
    g->m.set( examp, t_dirt, f_plant_seed );
    p.moves -= 500;
    add_msg( _( "Planted %s." ), item::nname( seed_id ).c_str() );
}

/**
 * If it's warm enough, pick one of the player's seeds and plant it.
 */
void iexamine::dirtmound( player &p, const tripoint &examp )
{

    if( !warm_enough_to_plant() ) {
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
void iexamine::harvest_plant( player &p, const tripoint &examp )
{
    if( g->m.i_at( examp ).empty() ) {
        g->m.i_clear( examp );
        g->m.furn_set( examp, f_null );
        debugmsg( "Missing seed in plant furniture!" );
        return;
    }
    const item &seed = g->m.i_at( examp ).front();
    if( !seed.is_seed() ) {
        debugmsg( "The seed item %s is not a seed!", seed.tname().c_str() );
        return;
    }

    const std::string &seedType = seed.typeId();
    if( seedType == "fungal_seeds" ) {
        fungus( p, examp );
        g->m.i_clear( examp );
    } else if( seedType == "marloss_seed" ) {
        fungus( p, examp );
        g->m.i_clear( examp );
        if( p.has_trait( trait_M_DEPENDENT ) && ( ( p.get_hunger() + p.get_starvation() > 500 ) ||
                p.get_thirst() > 300 ) ) {
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
        const itype &type = *seed.type;
        g->m.i_clear( examp );
        g->m.furn_set( examp, f_null );

        int skillLevel = p.get_skill_level( skill_survival );
        ///\EFFECT_SURVIVAL increases number of plants harvested from a seed
        int plantCount = rng( skillLevel / 2, skillLevel );
        if( plantCount >= 12 ) {
            plantCount = 12;
        } else if( plantCount <= 0 ) {
            plantCount = 1;
        }
        const int seedCount = std::max( 1l, rng( plantCount / 4, plantCount / 2 ) );
        for( auto &i : get_harvest_items( type, plantCount, seedCount, true ) ) {
            g->m.add_item_or_charges( examp, i );
        }
        p.moves -= 500;
    }
}

std::string iexamine::fertilize_failure_reason( player &p, const tripoint &tile,
        const itype_id &fertilizer )
{
    if( !g->m.has_flag_furn( "PLANT", tile ) ) {
        return _( "Tile isn't a plant" );
    }
    if( g->m.i_at( tile ).size() > 1 ) {
        return _( "Tile is already fertilized" );
    }
    if( !p.has_charges( fertilizer, 1 ) ) {
        return string_format( _( "Tried to fertilize with %s, but player doesn't have any." ),
                              fertilizer.c_str() );
    }

    return std::string();
}

void iexamine::fertilize_plant( player &p, const tripoint &tile, const itype_id &fertilizer )
{
    std::string reason = fertilize_failure_reason( p, tile, fertilizer );
    if( !reason.empty() ) {
        debugmsg( reason );
        return;
    }

    std::list<item> planted = p.use_charges( fertilizer, 1 );

    // Reduce the amount of time it takes until the next stage of the plant by
    // 20% of a seasons length. (default 2.8 days).
    const time_duration fertilizerEpoch = calendar::season_length() * 0.2;

    item &seed = g->m.i_at( tile ).front();
    //@todo: item should probably clamp the value on its own
    seed.set_birthday( std::max( calendar::time_of_cataclysm, seed.birthday() - fertilizerEpoch ) );
    // The plant furniture has the NOITEM token which prevents adding items on that square,
    // spawned items are moved to an adjacent field instead, but the fertilizer token
    // must be on the square of the plant, therefore this hack:
    const auto old_furn = g->m.furn( tile );
    g->m.furn_set( tile, f_null );
    g->m.spawn_item( tile, "fertilizer", 1, 1, calendar::turn );
    g->m.furn_set( tile, old_furn );
    p.mod_moves( -500 );

    add_msg( m_info, _( "You fertilize the %s with the %s." ), seed.get_plant_name().c_str(),
             planted.front().tname().c_str() );
}

itype_id iexamine::choose_fertilizer( player &p, const std::string &pname, bool ask_player )
{
    std::vector<const item *> f_inv = p.all_items_with_flag( "FERTILIZER" );
    if( f_inv.empty() ) {
        add_msg( m_info, _( "You have no fertilizer for the %s." ), pname.c_str() );
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

    if( ask_player && !query_yn( _( "Fertilize the %s" ), pname.c_str() ) ) {
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
    if( g->m.i_at( examp ).empty() ) {
        g->m.i_clear( examp );
        g->m.furn_set( examp, f_null );
        debugmsg( "Missing seed in plant furniture!" );
        return;
    }
    const item &seed = g->m.i_at( examp ).front();
    if( !seed.is_seed() ) {
        debugmsg( "The seed item %s is not a seed!", seed.tname().c_str() );
        return;
    }

    const std::string pname = seed.get_plant_name();

    if( g->m.furn( examp ) == f_plant_harvest && query_yn( _( "Harvest the %s?" ), pname.c_str() ) ) {
        harvest_plant( p, examp );
    } else if( g->m.furn( examp ) != f_plant_harvest ) {
        if( g->m.i_at( examp ).size() > 1 ) {
            add_msg( m_info, _( "This %s has already been fertilized." ), pname.c_str() );
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
                     false ).c_str() );
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
        g->draw_sidebar_messages(); // flush messages before popup
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

    auto items = g->m.i_at( examp );
    if( items.empty() ) {
        add_msg( _( "This kiln is empty..." ) );
        g->m.furn_set( examp, next_kiln_type );
        return;
    }
    auto char_type = item::find_type( "charcoal" );
    add_msg( _( "There's a charcoal kiln there." ) );
    const time_duration firing_time = 6_hours; // 5 days in real life
    const time_duration time_left = firing_time - items[0].age();
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
            items.push_back( *item_it );
            // This will add items to a space near the vat, because it's flagged as NOITEM.
            item_it = items.erase( item_it );
        } else {
            item_it++;
            brew_present = true;
        }
    }
    if( !brew_present ) {
        add_msg( _( "This keg is empty." ) );
        // @todo: Allow using brews from crafting inventory
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
        g->draw_sidebar_messages(); // flush messages before popup
        if( b_types.size() > 1 ) {
            b_index = uilist( _( "Use which brew?" ), b_names );
        } else { //Only one brew type was in inventory, so it's automatically used
            if( !query_yn( _( "Set %s in the vat?" ), b_names[0].c_str() ) ) {
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
        item &brew = g->m.i_at( examp ).front();
        brew_type = brew.typeId();
        brew_nname = item::nname( brew_type );
        charges_on_ground = brew.charges;
        add_msg( _( "This keg contains %s (%d), %0.f%% full." ),
                 brew.tname().c_str(), brew.charges, brew.volume() * 100.0 / vat_volume );
        g->draw_sidebar_messages(); // flush messages before popup
        enum options { ADD_BREW, REMOVE_BREW, START_FERMENT };
        uilist selectmenu;
        selectmenu.text = _( "Select an action" );
        selectmenu.addentry( ADD_BREW, ( p.charges_of( brew_type ) > 0 ), MENU_AUTOASSIGN,
                             string_format( _( "Add more %s to the vat" ), brew_nname.c_str() ) );
        selectmenu.addentry( REMOVE_BREW, brew.made_of( LIQUID ), MENU_AUTOASSIGN,
                             string_format( _( "Remove %s from the vat" ), brew.tname().c_str() ) );
        selectmenu.addentry( START_FERMENT, true, MENU_AUTOASSIGN, _( "Start fermenting cycle" ) );
        selectmenu.query();
        switch( selectmenu.ret ) {
            case ADD_BREW: {
                to_deposit = true;
                break;
            }
            case REMOVE_BREW: {
                g->handle_liquid_from_ground( g->m.i_at( examp ).begin(), examp );
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
        add_msg( _( "Set %s in the vat." ), brew_nname.c_str() );
        add_msg( _( "The keg now contains %s (%d), %0.f%% full." ),
                 brew.tname().c_str(), brew.charges, brew.volume() * 100.0 / vat_volume );
        g->m.i_clear( examp );
        //This is needed to bypass NOITEM
        g->m.add_item( examp, brew );
        p.moves -= 250;
        if( !vat_full ) {
            g->draw_sidebar_messages(); // flush messages before popup
            ferment = query_yn( _( "Start fermenting cycle?" ) );
        }
    }
    if( vat_full || ferment ) {
        g->m.i_at( examp ).front().set_age( 0_turns );
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
    auto items_here = g->m.i_at( examp );
    if( items_here.empty() ) {
        debugmsg( "fvat_full was empty!" );
        g->m.furn_set( examp, f_fvat_empty );
        return;
    }

    for( size_t i = 0; i < items_here.size(); i++ ) {
        auto &it = items_here[i];
        if( !it.made_of_from_type( LIQUID ) ) {
            add_msg( _( "You remove %s from the vat." ), it.tname().c_str() );
            g->m.add_item_or_charges( p.pos(), it );
            g->m.i_rem( examp, i );
            i--;
        }
    }

    if( items_here.empty() ) {
        g->m.furn_set( examp, f_fvat_empty );
        return;
    }

    item &brew_i = items_here.front();
    // Does the vat contain unfermented brew, or already fermented booze?
    // @todo: Allow "recursive brewing" to continue without player having to check on it
    if( brew_i.is_brewable() ) {
        add_msg( _( "There's a vat of %s set to ferment there." ), brew_i.tname().c_str() );

        //@todo: change brew_time to return time_duration
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

        g->draw_sidebar_messages(); // flush messages before popup
        if( query_yn( _( "Finish brewing?" ) ) ) {
            const auto results = brew_i.brewing_results();

            g->m.i_clear( examp );
            for( const auto &result : results ) {
                // @todo: Different age based on settings
                item booze( result, brew_i.birthday(), brew_i.charges );
                g->m.add_item( examp, booze );
                if( booze.made_of_from_type( LIQUID ) ) {
                    add_msg( _( "The %s is now ready for bottling." ), booze.tname().c_str() );
                }
            }

            p.moves -= 500;
            p.practice( skill_cooking, std::min( to_minutes<int>( brew_time ) / 10, 100 ) );
        }

        return;
    } else {
        add_msg( _( "There's a vat of fermented %s there." ), brew_i.tname().c_str() );
    }

    const std::string booze_name = items_here.front().tname();
    if( g->handle_liquid_from_ground( items_here.begin(), examp ) ) {
        g->m.furn_set( examp, f_fvat_empty );
        add_msg( _( "You squeeze the last drops of %s from the vat." ), booze_name.c_str() );
    }
}

//probably should move this functionality into the furniture JSON entries if we want to have more than a few "kegs"
static units::volume get_keg_capacity( const tripoint &pos )
{
    const furn_t &furn = g->m.furn( pos ).obj();
    if( furn.id == "f_standing_tank" )  {
        return units::from_liter( 300 );
    } else if( furn.id == "f_wood_keg" )  {
        return units::from_liter( 125 );
    }
    //add additional cases above
    else                                {
        return 0_ml;
    }
}

/**
 * Check whether there is a keg on the map that can be filled via @ref pour_into_keg.
 */
bool iexamine::has_keg( const tripoint &pos )
{
    return get_keg_capacity( pos ) > 0_ml;
}

void iexamine::keg( player &p, const tripoint &examp )
{
    none( p, examp );
    const auto keg_name = g->m.name( examp );
    units::volume keg_cap = get_keg_capacity( examp );
    bool liquid_present = false;
    for( int i = 0; i < static_cast<int>( g->m.i_at( examp ).size() ); i++ ) {
        if( !g->m.i_at( examp )[i].made_of_from_type( LIQUID ) || liquid_present ) {
            g->m.add_item_or_charges( examp, g->m.i_at( examp )[i] );
            g->m.i_rem( examp, i );
            i--;
        } else {
            liquid_present = true;
        }
    }
    if( !liquid_present ) {
        add_msg( m_info, _( "It is empty." ) );
        // Get list of all drinks
        auto drinks_inv = p.items_with( []( const item & it ) {
            return it.made_of( LIQUID );
        } );
        if( drinks_inv.empty() ) {
            add_msg( m_info, _( "You don't have any drinks to fill the %s with." ), keg_name.c_str() );
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
        g->draw_sidebar_messages(); // flush messages before popup
        if( drink_types.size() > 1 ) {
            drink_index = uilist( _( "Store which drink?" ), drink_names );
            if( drink_index < 0 || static_cast<size_t>( drink_index ) >= drink_types.size() ) {
                drink_index = -1;
            }
        } else { //Only one drink type was in inventory, so it's automatically used
            if( !query_yn( _( "Fill the %1$s with %2$s?" ),
                           keg_name.c_str(), drink_names[0].c_str() ) ) {
                drink_index = -1;
            }
        }
        if( drink_index < 0 ) {
            return;
        }
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
                     keg_name.c_str(), item::nname( drink_type ).c_str() );
        } else {
            add_msg( _( "You fill the %1$s with %2$s." ),
                     keg_name.c_str(), item::nname( drink_type ).c_str() );
        }
        p.moves -= 250;
        g->m.i_clear( examp );
        g->m.add_item( examp, drink );
        return;
    } else {
        auto drink = g->m.i_at( examp ).begin();
        const auto drink_tname = drink->tname();
        const auto drink_nname = item::nname( drink->typeId() );
        g->draw_sidebar_messages(); // flush messages before popup
        enum options {
            DISPENSE,
            HAVE_A_DRINK,
            REFILL,
            EXAMINE,
        };
        uilist selectmenu;
        selectmenu.addentry( DISPENSE, drink->made_of( LIQUID ), MENU_AUTOASSIGN,
                             _( "Dispense or dump %s" ), drink_tname.c_str() );
        selectmenu.addentry( HAVE_A_DRINK, drink->is_food() && drink->made_of( LIQUID ),
                             MENU_AUTOASSIGN, _( "Have a drink" ) );
        selectmenu.addentry( REFILL, true, MENU_AUTOASSIGN, _( "Refill" ) );
        selectmenu.addentry( EXAMINE, true, MENU_AUTOASSIGN, _( "Examine" ) );

        selectmenu.text = _( "Select an action" );
        selectmenu.query();

        switch( selectmenu.ret ) {
            case DISPENSE:
                if( g->handle_liquid_from_ground( drink, examp ) ) {
                    add_msg( _( "You squeeze the last drops of %1$s from the %2$s." ),
                             drink_tname.c_str(), keg_name.c_str() );
                }
                return;

            case HAVE_A_DRINK:
                if( !p.eat( *drink ) ) {
                    return; // They didn't actually drink
                }

                if( drink->charges == 0 ) {
                    add_msg( _( "You squeeze the last drops of %1$s from the %2$s." ),
                             drink_tname.c_str(), keg_name.c_str() );
                    g->m.i_clear( examp );
                }
                p.moves -= 250;
                return;

            case REFILL: {
                if( drink->volume() >= keg_cap ) {
                    add_msg( _( "The %s is completely full." ), keg_name.c_str() );
                    return;
                }
                int charges_held = p.charges_of( drink->typeId() );
                if( charges_held < 1 ) {
                    add_msg( m_info, _( "You don't have any %1$s to fill the %2$s with." ),
                             drink_nname.c_str(), keg_name.c_str() );
                    return;
                }
                item tmp( drink->typeId(), calendar::turn, charges_held );
                pour_into_keg( examp, tmp );
                p.use_charges( drink->typeId(), charges_held - tmp.charges );
                add_msg( _( "You fill the %1$s with %2$s." ), keg_name.c_str(), drink_nname.c_str() );
                p.moves -= 250;
                return;
            }

            case EXAMINE: {
                add_msg( m_info, _( "It contains %s (%d), %0.f%% full." ),
                         drink_tname.c_str(), drink->charges, drink->volume() * 100.0 / keg_cap );
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
        g->m.i_at( pos ).front().charges = 0; // Will be set later
    } else if( stack.front().typeId() != liquid.typeId() ) {
        add_msg( _( "The %s already contains some %s, you can't add a different liquid to it." ),
                 keg_name.c_str(), item::nname( stack.front().typeId() ).c_str() );
        return false;
    }

    item &drink = stack.front();
    if( drink.volume() >= keg_cap ) {
        add_msg( _( "The %s is full." ), keg_name.c_str() );
        return false;
    }

    add_msg( _( "You pour %1$s into the %2$s." ), liquid.tname().c_str(), keg_name.c_str() );
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
        !query_yn( _( "Harvest the %s?" ), g->m.tername( examp ).c_str() ) ) {
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
        !query_yn( _( "Dig up %s? This kills the tree!" ), g->m.tername( examp ).c_str() ) ) {
        return;
    }

    ///\EFFECT_SURVIVAL increases hickory root number per tree
    g->m.spawn_item( p.pos(), "hickory_root", rng( 1, 3 + p.get_skill_level( skill_survival ) ), 0,
                     calendar::turn );
    g->m.ter_set( examp, t_tree_hickory_dead );
    ///\EFFECT_SURVIVAL speeds up hickory root digging
    p.moves -= 2000 / ( p.get_skill_level( skill_survival ) + 1 ) + 100;
}

item_location maple_tree_sap_container()
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
                 item::nname( "tree_spile" ).c_str() );
        return;
    }

    std::vector<item_comp> comps;
    comps.push_back( item_comp( "tree_spile", 1 ) );
    p.consume_items( comps );

    p.mod_moves( -200 );
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
    bool has_container = false;
    long charges = 0;

    const std::string maple_sap_name = item::nname( "maple_sap" );

    auto items = g->m.i_at( examp );
    for( auto &it : items ) {
        if( it.is_bucket() || it.is_watertight_container() ) {
            has_container = true;

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
    selectmenu.addentry( ADD_CONTAINER, !has_container, MENU_AUTOASSIGN,
                         _( "Add a container to receive the %s" ), maple_sap_name.c_str() );
    selectmenu.addentry( HARVEST_SAP, has_sap, MENU_AUTOASSIGN, _( "Harvest current %s (%d)" ),
                         maple_sap_name.c_str(), charges );
    selectmenu.addentry( REMOVE_CONTAINER, has_container, MENU_AUTOASSIGN, _( "Remove container" ) );

    selectmenu.text = _( "Select an action" );
    selectmenu.query();

    switch( selectmenu.ret ) {
        case REMOVE_TAP: {
            if( !p.has_quality( quality_id( "HAMMER" ) ) ) {
                add_msg( m_info, _( "You need a hammering tool to remove the spile from the crust." ) );
                return;
            }

            item tree_spile( "tree_spile" );
            add_msg( _( "You remove the %s." ), tree_spile.tname( 1 ).c_str() );
            g->m.add_item_or_charges( p.pos(), tree_spile );

            for( auto &it : items ) {
                g->m.add_item_or_charges( p.pos(), it );
            }
            g->m.i_clear( examp );

            p.mod_moves( -200 );
            g->m.ter_set( examp, t_tree_maple );

            return;
        }

        case ADD_CONTAINER: {
            auto cont_loc = maple_tree_sap_container();

            item *container = cont_loc.get_item();
            if( container ) {
                g->m.add_item_or_charges( examp, *container, false );

                cont_loc.remove_item();
            } else {
                add_msg( m_info, _( "No container added. The sap will just spill on the ground." ) );
            }

            return;
        }

        case HARVEST_SAP:
            for( auto &it : items ) {
                if( ( it.is_bucket() || it.is_watertight_container() ) && !it.is_container_empty() ) {
                    auto &liquid = it.contents.front();
                    if( liquid.typeId() == "maple_sap" ) {
                        g->handle_liquid_from_container( it, PICKUP_RANGE );
                    }
                }
            }

            return;

        case REMOVE_CONTAINER: {
            g->u.assign_activity( activity_id( "ACT_PICKUP" ) );
            g->u.activity.placement = examp - p.pos();
            g->u.activity.values.push_back( false );
            g->u.activity.values.push_back( 0 );
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
        !query_yn( _( "Forage through %s?" ), g->m.tername( examp ).c_str() ) ) {
        none( p, examp );
        return;
    }

    add_msg( _( "You forage through the %s." ), g->m.tername( examp ).c_str() );
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
                     input.tname().c_str(), m.name().c_str() );
            return;
        }
        if( input.is_container() && !input.is_container_empty() ) {
            //~ %1$s: an item in the compactor
            add_msg( _( "You realize this isn't going to work because %1$s has not been emptied of its contents." ),
                     input.tname().c_str() );
            return;
        }
        sum_weight += input.weight();
    }
    if( sum_weight <= 0_gram ) {
        //~ %1$s: desired compactor output material
        add_msg( _( "There is no %1$s in the compactor.  Drop some metal items onto it and try again." ),
                 m.name().c_str() );
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
                                        convert_weight( sum_weight ), weight_units(), m.name().c_str() );
    for( auto &ci : m.compacts_into() ) {
        auto it = item( ci, 0, item::solitary_tag{} );
        const int amount = norm_recover_weight / it.weight();
        //~ %1$d: number of, %2$s: output item
        choose_output.addentry( string_format( _( "about %1$d %2$s" ), amount,
                                               it.tname( amount ).c_str() ) );
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
    sounds::sound( examp, 80, sounds::sound_t::combat, _( "Ka-klunk!" ) );
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
        add_msg( _( "The compactor beeps: \"No %1$s to process!\"" ), m.name().c_str() );
        return;
    }
    if( !out_desired ) {
        //~ %1$s: compactor output material
        add_msg( _( "The compactor beeps: \"Insufficient %1$s!\"" ), m.name().c_str() );
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
    if( seen && possible >= 99 ) {
        add_msg( m_info, _( "That %s looks too dangerous to mess with. Best leave it alone." ),
                 tr.name().c_str() );
        return;
    }
    // Some traps are not actual traps. Those should get a different query.
    if( seen && possible == 0 &&
        tr.get_avoidance() == 0 ) { // Separated so saying no doesn't trigger the other query.
        if( query_yn( _( "There is a %s there. Take down?" ), tr.name().c_str() ) ) {
            g->m.disarm_trap( examp );
        }
    } else if( seen && query_yn( _( "There is a %s there.  Disarm?" ), tr.name().c_str() ) ) {
        g->m.disarm_trap( examp );
    }
}

void iexamine::water_source( player &p, const tripoint &examp )
{
    item water = g->m.water_from( examp );
    ( void ) p; // @todo: use me
    g->handle_liquid( water, nullptr, 0, &examp );
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
    if( pseudo->tool && !pseudo->tool->ammo_id.is_null() ) {
        return item::find_type( pseudo->tool->ammo_id->default_ammotype() );
    }
    return nullptr;
}

static long count_charges_in_list( const itype *type, const map_stack &items )
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
        add_msg( m_info, _( "This %s can not be reloaded!" ), f.name().c_str() );
        return;
    }
    const int amount_in_furn = count_charges_in_list( ammo, g->m.i_at( examp ) );
    if( amount_in_furn > 0 ) {
        //~ %1$s - furniture, %2$d - number, %3$s items.
        add_msg( _( "The %1$s contains %2$d %3$s." ), f.name().c_str(), amount_in_furn,
                 ammo->nname( amount_in_furn ).c_str() );
    }
    const int max_amount_in_furn = ammo->charges_per_volume( f.max_volume );
    const int max_reload_amount = max_amount_in_furn - amount_in_furn;
    if( max_reload_amount <= 0 ) {
        return;
    }
    const int amount_in_inv = p.charges_of( ammo->get_id() );
    if( amount_in_inv == 0 ) {
        //~ Reloading or restocking a piece of furniture, for example a forge.
        add_msg( m_info, _( "You need some %1$s to reload this %2$s." ), ammo->nname( 2 ).c_str(),
                 f.name().c_str() );
        return;
    }
    const long max_amount = std::min( amount_in_inv, max_reload_amount );
    //~ Loading fuel or other items into a piece of furniture.
    const std::string popupmsg = string_format( _( "Put how many of the %1$s into the %2$s?" ),
                                 ammo->nname( max_amount ).c_str(), f.name().c_str() );
    long amount = string_input_popup()
                  .title( popupmsg )
                  .width( 20 )
                  .text( to_string( max_amount ) )
                  .only_digits( true )
                  .query_long();
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
    add_msg( _( "You reload the %s." ), g->m.furnname( examp ).c_str() );
    p.moves -= 100;
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
        p.moves -= 200;
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
    // Chose spray can because it seems appropriate.
    int required_writing_charges = 1;
    if( p.has_charges( "spray_can", required_writing_charges ) ) {
        // Different messages if the sign already has writing associated with it.
        std::string query_message = previous_signage_exists ?
                                    _( "Overwrite the existing message on the sign with spray paint?" ) :
                                    _( "Add a message to the sign with spray paint?" );
        std::string spray_painted_message = previous_signage_exists ?
                                            _( "You overwrite the previous message on the sign with your graffiti" ) :
                                            _( "You graffiti a message onto the sign." );
        std::string ignore_message = _( "You leave the sign alone." );
        if( query_yn( query_message.c_str() ) ) {
            std::string signage = string_input_popup()
                                  .title( _( "Spray what?" ) )
                                  .identifier( "signage" )
                                  .query_string();
            if( signage.empty() ) {
                p.add_msg_if_player( m_neutral, ignore_message.c_str() );
            } else {
                g->m.set_signage( examp, signage );
                p.add_msg_if_player( m_info, spray_painted_message.c_str() );
                p.moves -= 2 * signage.length();
                p.use_charges( "spray_can", required_writing_charges );
            }
        } else {
            p.add_msg_if_player( m_neutral, ignore_message.c_str() );
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

static cata::optional<tripoint> getNearFilledGasTank( const tripoint &center, long &gas_units )
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

static long getGasPricePerLiter( int discount )
{
    // Those prices are in cents
    static const long prices[4] = { 1400, 1320, 1200, 1000 };
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

static bool toPumpFuel( const tripoint &src, const tripoint &dst, long units )
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

static long fromPumpFuel( const tripoint &dst, const tripoint &src )
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
            long amount = item_it->charges;
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
        popup( str_to_illiterate_str( _( "Failure! No gas pumps found!" ) ).c_str() );
        return;
    }

    long tankGasUnits;
    const cata::optional<tripoint> pTank_ = getNearFilledGasTank( examp, tankGasUnits );
    if( !pTank_ ) {
        popup( str_to_illiterate_str( _( "Failure! No gas tank found!" ) ).c_str() );
        return;
    }
    const tripoint pTank = *pTank_;

    if( tankGasUnits == 0 ) {
        popup( str_to_illiterate_str(
                   _( "This station is out of fuel.  We apologize for the inconvenience." ) ).c_str() );
        return;
    }

    if( uistate.ags_pay_gas_selected_pump + 1 > pumpCount ) {
        uistate.ags_pay_gas_selected_pump = 0;
    }

    int discount = findBestGasDiscount( p );
    std::string discountName = getGasDiscountName( discount );

    long pricePerUnit = getGasPricePerLiter( discount );

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
        long money = p.charges_of( "cash_card" );

        if( money < pricePerUnit ) {
            popup( str_to_illiterate_str(
                       _( "Not enough money, please refill your cash card." ) ).c_str() ); //or ride on a solar car, ha ha ha
            return;
        }

        long maximum_liters = std::min( money / pricePerUnit, tankGasUnits / 1000 );

        std::string popupmsg = string_format(
                                   _( "How many liters of gasoline to buy? Max: %d L. (0 to cancel) " ), maximum_liters );
        long liters = string_input_popup()
                      .title( popupmsg )
                      .width( 20 )
                      .text( to_string( maximum_liters ) )
                      .only_digits( true )
                      .query_long();
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

        sounds::sound( p.pos(), 6, sounds::sound_t::activity, _( "Glug Glug Glug" ) );

        long cost = liters * pricePerUnit;
        money -= cost;
        p.use_charges( "cash_card", cost );

        add_msg( m_info, _( "Your cash cards now hold %s." ), format_money( money ) );
        p.moves -= 100;
        return;
    }

    if( hack == choice ) {
        switch( hack_attempt( p ) ) {
            case HACK_UNABLE:
                break;
            case HACK_FAIL:
                p.add_memorial_log( pgettext( "memorial_male", "Set off an alarm." ),
                                    pgettext( "memorial_female", "Set off an alarm." ) );
                sounds::sound( p.pos(), 60, sounds::sound_t::music, _( "an alarm sound!" ) );
                if( examp.z > 0 && !g->events.queued( EVENT_WANTED ) ) {
                    g->events.add( EVENT_WANTED, calendar::turn + 30_minutes, 0, p.global_sm_location() );
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
                                   _( "Glug Glug Glug Glug Glug Glug Glug Glug Glug" ) );
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
        long amount = pGasPump ? fromPumpFuel( pTank, *pGasPump ) : 0l;
        if( amount >= 0 ) {
            sounds::sound( p.pos(), 6, sounds::sound_t::activity, _( "Glug Glug Glug" ) );
            cashcard->charges += amount * pricePerUnit / 1000.0f;
            add_msg( m_info, _( "Your cash cards now hold %s." ), format_money( p.charges_of( "cash_card" ) ) );
            p.moves -= 100;
            return;
        } else {
            popup( _( "Unable to refund, no fuel in pump." ) );
            return;
        }
    }
}

void iexamine::climb_down( player &p, const tripoint &examp )
{
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

    p.moves -= 100 + 100 * fall_mod;
    p.setpos( examp );
    if( climb_cost > 0 || rng_float( 0.8, 1.0 ) > fall_mod ) {
        // One tile of falling less (possibly zero)
        g->vertical_move( -1, true );
    }

    g->m.creature_on_trap( p );
}

player &player_on_couch( player &p, const tripoint &autodoc_loc, player &null_patient,
                         bool &adjacent_couch )
{
    for( const auto &couch_loc : g->m.points_in_radius( autodoc_loc, 1, 0 ) ) {
        const furn_str_id couch( "f_autodoc_couch" );
        if( g->m.furn( couch_loc ) == couch ) {
            adjacent_couch = true;
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

player &best_installer( player &p, player &null_player, int difficulty )
{
    float player_skill = p.bionics_adjusted_skill( skill_firstaid,
                         skill_computer,
                         skill_electronics );

    std::vector< std::pair<float, long>> ally_skills;
    ally_skills.reserve( g->allies().size() );
    for( size_t i = 0; i < g->allies().size() ; i ++ ) {
        std::pair<float, long> ally_skill;
        const npc *e = g->allies()[ i ];

        player &ally = *g->critter_by_id<player>( e->getID() );
        ally_skill.second = i;
        ally_skill.first = ally.bionics_adjusted_skill( skill_firstaid,
                           skill_computer,
                           skill_electronics );
        ally_skills.push_back( ally_skill );
    }
    std::sort( ally_skills.begin(), ally_skills.end(), [&]( const std::pair<float, long> &lhs,
    const std::pair<float, long> &rhs ) {
        return rhs.first < lhs.first;
    } );
    int player_cos = bionic_manip_cos( player_skill, true, difficulty );
    for( size_t i = 0; i < g->allies().size() ; i ++ ) {
        if( ally_skills[ i ].first > player_skill ) {
            const npc *e = g->allies()[ ally_skills[ i ].second ];
            player &ally = *g->critter_by_id<player>( e->getID() );
            int ally_cos = bionic_manip_cos( ally_skills[ i ].first, true, difficulty );
            if( e->has_effect( effect_sleep ) ) {
                //~ %1$s is the name of the ally
                if( !g->u.query_yn( string_format(
                                        _( "<color_white>%1$s is asleep, but has a <color_green>%2$d<color_white> chance of success compared to your <color_red>%3$d<color_white> chance of success.  Continue with a higher risk of failure?</color>" ),
                                        ally.disp_name(), ally_cos, player_cos ) ) ) {
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
    };

    bool adjacent_couch = false;
    static player null_player;
    player &patient = player_on_couch( p, examp, null_player, adjacent_couch );

    if( !adjacent_couch ) {
        popup( _( "No connected couches found.  Operation impossible.  Exiting." ) );
        return;
    }
    if( &patient == &null_player ) {
        popup( _( "No patient found located on the connected couches.  Operation impossible.  Exiting." ) );
        return;
    }

    bool needs_anesthesia = true;
    std::vector<item_comp> acomps;
    if( p.has_trait( trait_NOPAIN ) || p.has_bionic( bionic_id( "bio_painkiller" ) ) ) {
        needs_anesthesia = false;
    } else {
        std::vector<const item *> a_filter = p.crafting_inventory().items_with( []( const item & it ) {
            return it.has_flag( "ANESTHESIA" );
        } );
        for( const item *anesthesia_item : a_filter ) {
            acomps.push_back( item_comp( anesthesia_item->typeId(), 1 ) );
        }
        if( acomps.empty() ) {
            popup( _( "You need an anesthesia kit for autodoc to perform any operation." ) );
            return;
        }
    }

    uilist amenu;
    amenu.text = _( "Autodoc Mk. XI.  Status: Online.  Please choose operation." );
    amenu.addentry( INSTALL_CBM, true, 'i', _( "Choose Compact Bionic Module to install." ) );
    amenu.addentry( UNINSTALL_CBM, true, 'u', _( "Choose installed bionic to uninstall." ) );

    amenu.query();

    switch( amenu.ret ) {
        case INSTALL_CBM: {
            const item_location bionic = g->inv_map_splice( []( const item & e ) {
                return e.is_bionic();
            }, _( "Choose CBM to install" ), PICKUP_RANGE, _( "You don't have any CBMs to install." ) );

            if( !bionic ) {
                return;
            }

            const item *it = bionic.get_item();
            const itype *itemtype = it->type;
            const bionic_id &bid = itemtype->bionic->id;

            if( patient.is_npc() && !bid->npc_usable ) {
                //~ %1$s is the bionic CBM display name, %2$s is the patient name
                popup( _( "%1$s cannot be installed on %2$s." ), it->display_name().c_str(), patient.name );
                return;
            }

            if( patient.has_bionic( bid ) ) {
                //~ %1$s is patient name
                popup_player_or_npc( patient, _( "You have already installed this bionic." ),
                                     _( "%1$s has already installed this bionic." ) );
                return;
            } else if( bid->upgraded_bionic && !patient.has_bionic( bid->upgraded_bionic ) &&
                       it->is_upgrade() ) {
                //~ %1$s is patient name
                popup_player_or_npc( patient, _( "You have no base version of this bionic to upgrade." ),
                                     _( "%1$s has no base version of this bionic to upgrade." ) );
                return;
            } else {
                const bool downgrade = std::any_of( bid->available_upgrades.begin(),
                                                    bid->available_upgrades.end(),
                                                    std::bind( &player::has_bionic, &patient,
                                                            std::placeholders::_1 ) );
                if( downgrade ) {
                    //~ %1$s is patient name
                    popup_player_or_npc( patient, _( "You have already installed a superior version of this bionic." ),
                                         _( "%1$s has installed a superior version of this bionic." ) );
                    return;
                }
            }
            player &installer = best_installer( p, null_player, itemtype->bionic->difficulty );
            if( &installer == &null_player ) {
                return;
            }

            const time_duration duration = itemtype->bionic->difficulty * 20_minutes;
            if( patient.install_bionics( ( *itemtype ), installer, true ) ) {
                patient.introduce_into_anesthesia( duration, installer, needs_anesthesia );
                std::vector<item_comp> comps;
                comps.push_back( item_comp( it->typeId(), 1 ) );
                p.consume_items( comps, 1 );
                if( needs_anesthesia ) {
                    p.consume_items( acomps, 1 );
                }
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

            item bionic_to_uninstall;
            std::vector<itype_id> bionic_types;
            std::vector<std::string> bionic_names;

            for( auto &bio : installed_bionics ) {
                if( std::find( bionic_types.begin(), bionic_types.end(), bio.id.str() ) == bionic_types.end() ) {
                    if( bio.id != bionic_id( "bio_power_storage" ) ||
                        bio.id != bionic_id( "bio_power_storage_mkII" ) ) {
                        const auto &bio_data = bio.info();
                        bionic_names.push_back( bio_data.name );
                        bionic_types.push_back( bio.id.str() );
                        if( item::type_is_defined( bio.id.str() ) ) {
                            bionic_to_uninstall = item( bio.id.str(), 0 );
                        }
                    }
                }
            }

            int bionic_index = uilist( _( "Choose bionic to uninstall" ), bionic_names );
            if( bionic_index < 0 ) {
                return;
            }

            const itype *itemtype = bionic_to_uninstall.type;
            // Malfunctioning bionics don't have associated items and get a difficulty of 12
            const int difficulty = itemtype->bionic ? itemtype->bionic->difficulty : 12;
            const time_duration duration = difficulty * 20_minutes;

            player &installer = best_installer( p, null_player, difficulty );
            if( &installer == &null_player ) {
                return;
            }

            if( patient.uninstall_bionic( bionic_id( bionic_types[bionic_index] ), installer, true ) ) {
                patient.introduce_into_anesthesia( duration, installer, needs_anesthesia );
                if( needs_anesthesia ) {
                    p.consume_items( acomps, 1 );
                }
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
}

static int get_charcoal_charges( units::volume food )
{
    const int charcoal = to_liter( food ) * sm_rack::CHARCOAL_PER_LITER;

    return  std::max( charcoal, sm_rack::MIN_CHARCOAL );
}

void smoker_activate( player &p, const tripoint &examp )
{
    furn_id cur_smoker_type = g->m.furn( examp );
    furn_id next_smoker_type = f_null;
    if( cur_smoker_type == f_smoking_rack ) {
        next_smoker_type = f_smoking_rack_active;
    } else {
        debugmsg( "Examined furniture has action smoker_activate, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }
    bool food_present = false;
    bool charcoal_present = false;
    auto items = g->m.i_at( examp );
    units::volume food_volume = 0_ml;
    item *charcoal = nullptr;

    for( size_t i = 0; i < items.size(); i++ ) {
        auto &it = items[i];
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
                     false ).c_str() );
            add_msg( _( "You remove %s from the rack." ), it.tname().c_str() );
            g->m.add_item_or_charges( p.pos(), it );
            g->m.i_rem( examp, i );
            i--;
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
    if( food_volume > sm_rack::MAX_FOOD_VOLUME ) {
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
            // Do one final rot check before smoking, then apply the smoking FLAG to prevent further checks.
            it.calc_rot( examp );
            it.set_flag( "SMOKING" );
        }
    }
    g->m.furn_set( examp, next_smoker_type );
    if( charcoal->charges == char_charges ) {
        g->m.i_rem( examp, charcoal );
    } else {
        charcoal->charges -= char_charges;
    }
    item result( "fake_smoke_plume", calendar::turn );
    result.item_counter = 3600; // = 6 hours
    result.activate();
    g->m.add_item( examp, result );
    add_msg( _( "You light a small fire under the rack and it starts to smoke." ) );
}

void smoker_finalize( player &, const tripoint &examp, const time_point &start_time )
{
    furn_id cur_smoker_type = g->m.furn( examp );
    furn_id next_smoker_type = f_null;
    if( cur_smoker_type == f_smoking_rack_active ) {
        next_smoker_type = f_smoking_rack;
    } else {
        debugmsg( "Furniture executed action smoker_finalize, but is of type %s",
                  g->m.furn( examp ).id().c_str() );
        return;
    }

    auto items = g->m.i_at( examp );
    if( items.empty() ) {
        g->m.furn_set( examp, next_smoker_type );
        return;
    }

    for( auto &it : items ) {
        if( it.has_flag( "SMOKABLE" ) ) { // Don't check charcoal
            it.calc_rot_while_smoking( examp, 6_hours );
        }
    }

    for( size_t i = 0; i < items.size(); i++ ) {
        auto &item_it = items[i];
        if( item_it.type->comestible ) {
            if( item_it.type->comestible->smoking_result.empty() ) {
                item_it.unset_flag( "SMOKING" );
            } else {
                item result( item_it.type->comestible->smoking_result, start_time + 6_hours, item_it.charges );

                // Set flag to tell set_relative_rot() to calc from bday not now
                result.set_flag( "SMOKING_RESULT" );
                result.set_relative_rot( item_it.get_relative_rot() );
                result.unset_flag( "SMOKING_RESULT" );
                item_it = result;
            }
        }
    }
    g->m.furn_set( examp, next_smoker_type );
}

void smoker_load_food( player &p, const tripoint &examp, const units::volume &remaining_capacity )
{
    std::vector<item_comp> comps;

    if( g->m.furn( examp ) == furn_str_id( "f_smoking_rack_active" ) ) {
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
    int count = 0;
    std::list<std::string> names;
    std::vector<const item *> entries;
    for( const item *smokable_item : filtered ) {

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
        count = 0;
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
    count = 0;
    auto what = entries[smenu.ret];
    for( auto c : comps ) {
        if( c.type == what->typeId() ) {
            count = c.count;
        }
    }

    const int max_count_for_capacity =  remaining_capacity / what->base_volume();
    const int max_count = std::min( count, max_count_for_capacity );

    // ... then ask how many to put it
    const std::string popupmsg = string_format( _( "Insert how many %s into the rack?" ),
                                 item::nname( what->typeId(), count ).c_str() );
    long amount = string_input_popup()
                  .title( popupmsg )
                  .width( 20 )
                  .text( to_string( max_count ) )
                  .only_digits( true )
                  .query_long();

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
    inv.form_from_map( g->u.pos(), PICKUP_RANGE );
    inv.remove_items_with( []( const item & it ) {
        return it.rotten();
    } );
    comp_selection<item_comp> selected = p.select_item_component( comps, 1, inv, true );
    std::list<item> moved = p.consume_items( selected, 1 );

    // hack, because consume_items doesn't seem to care of what item is consumed despite filters
    // TODO: find a way to filter out rotten items from those actualy consumed
    bool rotted = false;
    for( const item &m : moved ) {
        if( m.rotten() ) {
            rotted = true;
        }
    }
    if( rotted ) {
        add_msg( m_info, _( "You have rotten food mixed with fresh.  Get rid of it first." ) );
        for( const item &m : moved ) {
            g->m.add_item( p.pos(), m );
            p.mod_moves( -p.item_handling_cost( m ) );
        }
        p.invalidate_crafting_inventory();
        return;
    }

    for( item m : moved ) {
        g->m.add_item( examp, m );
        p.mod_moves( -p.item_handling_cost( m ) );
        add_msg( m_info, _( "You carefully place %s %s in the rack." ), amount, m.nname( m.typeId(),
                 amount ) );
    }
    p.invalidate_crafting_inventory();
}

void iexamine::on_smoke_out( const tripoint &examp, const time_point &start_time )
{
    if( g->m.furn( examp ) == furn_str_id( "f_smoking_rack_active" ) ) {
        smoker_finalize( g->u, examp, start_time );
    }
}

void iexamine::smoker_options( player &p, const tripoint &examp )
{
    bool active = g->m.furn( examp ) == furn_str_id( "f_smoking_rack_active" );
    auto items_here = g->m.i_at( examp );

    if( items_here.empty() && active ) {
        debugmsg( "f_smoking_rack_active was empty!" );
        g->m.furn_set( examp, f_smoking_rack );
        return;
    }

    if( items_here.size() == 1 && items_here.begin()->typeId() == "fake_smoke_plume" ) {
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

    for( size_t i = 0; i < items_here.size(); i++ ) {
        auto &it = items_here[i];
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
    const auto remaining_capacity = sm_rack::MAX_FOOD_VOLUME - f_volume;
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

        smenu.addentry_desc( 2, !full, 'f',
                             full ? _( "Insert food for smoking... smoking rack is full" ) :
                             string_format( _( "Insert food for smoking... remaining capacity is %s %s" ),
                                            format_volume( remaining_capacity ), volume_units_abbr() ),
                             _( "Fill the smoking rack with raw meat, fish or sausages for smoking or fruit or vegetable or smoked meat for drying." ) );

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
                        pop << _( "It will finish smoking in less than an hour." );
                    } else {
                        pop << string_format( _( "It should take about %d minutes to finish smoking." ), minutes_left );
                    }
                }
            } else {
                pop << "<color_green>" << _( "There's a smoking rack here." ) << "</color>" << "\n";
            }
            pop << "<color_green>" << _( "You inspect its contents and find: " ) << "</color>" << "\n \n ";
            if( items_here.empty() ) {
                pop << _( "... that it is empty." );
            } else {
                for( size_t i = 0; i < items_here.size(); i++ ) {
                    auto &it = items_here[i];
                    if( it.typeId() == "fake_smoke_plume" ) {
                        pop << "\n " << "<color_red>" << _( "You see some smoldering embers there." ) << "</color>" <<
                            "\n ";
                        continue;
                    }
                    pop << "-> " << it.nname( it.typeId(), it.charges );
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
            smoker_load_food( p, examp, remaining_capacity );
            break;
        case 3: // load charcoal
            reload_furniture( p, examp );
            break;
        case 4: // remove food
            rem_f_opt = true;
        /* fallthrough */
        case 5: { //remove charcoal
            for( size_t i = 0; i < items_here.size(); i++ ) {
                auto &it = items_here[i];
                if( ( rem_f_opt && it.is_food() ) || ( !rem_f_opt && ( it.typeId() == "charcoal" ) ) ) {
                    // get handling cost before the item reference is invalidated
                    const int handling_cost = -p.item_handling_cost( it );

                    add_msg( _( "You remove %s from the rack." ), it.tname().c_str() );
                    g->m.add_item_or_charges( p.pos(), it );
                    g->m.i_rem( examp, i );
                    p.mod_moves( handling_cost );
                    i--;
                }
            }
            if( active && rem_f_opt ) {
                g->m.furn_set( examp, f_smoking_rack );
                add_msg( m_info, _( "You stop the smoking process." ) );
            }
        }
        break;
        default:
            add_msg( m_info, _( "Never mind." ) );
            break;
        case 7:
            g->m.furn_set( examp, f_smoking_rack );
            add_msg( m_info, _( "You stop the smoking process." ) );
            break;
    }
}

void iexamine::open_safe( player &, const tripoint &examp )
{
    add_msg( m_info, _( "You open the unlocked safe. " ) );
    g->m.furn_set( examp, f_safe_o );
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
            { "rubble", &iexamine::rubble },
            { "crate", &iexamine::crate },
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
            { "fungus", &iexamine::fungus },
            { "flower_spurge", &iexamine::flower_spurge },
            { "flower_tulip", &iexamine::flower_tulip },
            { "flower_bluebell", &iexamine::flower_bluebell },
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
            { "tree_marloss", &iexamine::tree_marloss },
            { "tree_hickory", &iexamine::tree_hickory },
            { "tree_maple", &iexamine::tree_maple },
            { "tree_maple_tapped", &iexamine::tree_maple_tapped },
            { "shrub_wildveggies", &iexamine::shrub_wildveggies },
            { "recycle_compactor", &iexamine::recycle_compactor },
            { "trap", &iexamine::trap },
            { "water_source", &iexamine::water_source },
            { "reload_furniture", &iexamine::reload_furniture },
            { "curtains", &iexamine::curtains },
            { "sign", &iexamine::sign },
            { "pay_gas", &iexamine::pay_gas },
            { "gunsafe_ml", &iexamine::gunsafe_ml },
            { "gunsafe_el", &iexamine::gunsafe_el },
            { "locked_object", &iexamine::locked_object },
            { "kiln_empty", &iexamine::kiln_empty },
            { "kiln_full", &iexamine::kiln_full },
            { "climb_down", &iexamine::climb_down },
            { "autodoc", &iexamine::autodoc },
            { "smoker_options", &iexamine::smoker_options },
            { "open_safe", &iexamine::open_safe }
        }
    };

    auto iter = function_map.find( function_name );
    if( iter != function_map.end() ) {
        return iter->second;
    }

    //No match found
    debugmsg( "Could not find an iexamine function matching '%s'!", function_name.c_str() );
    return &iexamine::none;
}

hack_result iexamine::hack_attempt( player &p )
{
    if( p.has_trait( trait_ILLITERATE ) ) {
        return HACK_UNABLE;
    }
    bool using_electrohack = ( p.has_charges( "electrohack", 25 ) &&
                               query_yn( _( "Use electrohack?" ) ) );
    bool using_fingerhack = ( !using_electrohack && p.has_bionic( bionic_id( "bio_fingerhack" ) ) &&
                              p.power_level  > 24  && query_yn( _( "Use fingerhack?" ) ) );

    if( !( using_electrohack || using_fingerhack ) ) {
        return HACK_UNABLE;
    }

    p.moves -= 500;
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
    success += rng( 0, int( ( p.int_cur - 8 ) / 2 ) );

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
