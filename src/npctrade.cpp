#include "npctrade.h"

#include <limits.h>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <memory>
#include <set>

#include "avatar.h"
#include "debug.h"
#include "cata_utility.h"
#include "game.h"
#include "input.h"
#include "map_selector.h"
#include "npc.h"
#include "output.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"
#include "vehicle_selector.h"
#include "color.h"
#include "cursesdef.h"
#include "item.h"
#include "player.h"
#include "string_input_popup.h"
#include "units.h"
#include "visitable.h"
#include "type_id.h"
#include "faction.h"
#include "pimpl.h"

const skill_id skill_barter( "barter" );

void npc_trading::transfer_items( std::vector<item_pricing> &stuff, player &giver,
                                  player &receiver, std::list<item_location *> &from_map,
                                  bool npc_gives )
{
    for( item_pricing &ip : stuff ) {
        if( !ip.selected ) {
            continue;
        }

        if( ip.loc.get_item() == nullptr ) {
            DebugLog( D_ERROR, D_NPC ) << "Null item being traded in npc_trading::transfer_items";
            continue;
        }

        item gift = *ip.loc.get_item();
        gift.set_owner( receiver );
        int charges = npc_gives ? ip.u_charges : ip.npc_charges;
        int count = npc_gives ? ip.u_has : ip.npc_has;

        if( ip.charges ) {
            gift.charges = charges;
            receiver.i_add( gift );
        } else {
            for( int i = 0; i < count; i++ ) {
                receiver.i_add( gift );
            }
        }

        if( ip.loc.where() == item_location::type::character ) {
            if( gift.typeId() == giver.weapon.typeId() ) {
                giver.remove_weapon();
            }
            if( ip.charges > 0 ) {
                giver.use_charges( gift.typeId(), charges );
            } else if( ip.count > 0 ) {
                for( int i = 0; i < count; i++ ) {
                    giver.use_amount( gift.typeId(), 1 );
                }
            }
        } else {
            from_map.push_back( &ip.loc );
        }
    }
}

std::vector<item_pricing> npc_trading::init_selling( npc &np )
{
    std::vector<item_pricing> result;
    invslice slice = np.inv.slice();
    for( auto &i : slice ) {
        item &it = i->front();

        const int price = it.price( true );
        int val = np.value( it );
        if( np.wants_to_sell( it, val, price ) ) {
            result.emplace_back( np, i->front(), val, i->size() );
        }
    }

    if(
        np.will_exchange_items_freely() &&
        ! np.weapon.is_null() &&
        ! np.weapon.has_flag( "NO_UNWIELD" )
    ) {
        result.emplace_back( np, np.weapon, np.value( np.weapon ), false );
    }

    return result;
}

double npc_trading::net_price_adjustment( const player &buyer, const player &seller )
{
    // Adjust the prices based on your barter skill.
    // cap adjustment so nothing is ever sold below value
    ///\EFFECT_INT_NPC slightly increases bartering price changes, relative to your INT

    ///\EFFECT_BARTER_NPC increases bartering price changes, relative to your BARTER

    ///\EFFECT_INT slightly increases bartering price changes, relative to NPC INT

    ///\EFFECT_BARTER increases bartering price changes, relative to NPC BARTER
    double adjust = 0.05 * ( seller.int_cur - buyer.int_cur ) +
                    price_adjustment( seller.get_skill_level( skill_barter ) -
                                      buyer.get_skill_level( skill_barter ) );
    return( std::max( adjust, 1.0 ) );
}

template <typename T, typename Callback>
void buy_helper( T &src, Callback cb )
{
    src.visit_items( [&src, &cb]( item * node ) {
        cb( std::move( item_location( src, node ) ), 1 );

        return VisitResponse::SKIP;
    } );
}

std::vector<item_pricing> npc_trading::init_buying( player &buyer, player &seller, bool is_npc )
{
    std::vector<item_pricing> result;
    npc *np_p = dynamic_cast<npc *>( &buyer );
    if( is_npc ) {
        np_p = dynamic_cast<npc *>( &seller );
    }
    npc &np = *np_p;
    faction *fac = np.get_faction();

    double adjust = net_price_adjustment( buyer, seller );

    const auto check_item = [fac, adjust, is_npc, &np, &result, &seller]( item_location &&
    loc, int count = 1 ) {
        item *it_ptr = loc.get_item();
        if( it_ptr == nullptr || it_ptr->is_null() ) {
            return;
        }
        item &it = *it_ptr;

        // Don't sell items we don't own.
        if( !it.is_owned_by( seller ) ) {
            return;
        }

        const int market_price = it.price( true );
        int val = np.value( it, market_price );
        if( ( is_npc && np.wants_to_sell( it, val, market_price ) ) ||
            np.wants_to_buy( it, val, market_price ) ) {
            result.emplace_back( std::move( loc ), val, count );
            result.back().adjust_values( adjust, fac );
        }
    };

    invslice slice = seller.inv.slice();
    for( auto &i : slice ) {
        check_item( item_location( seller, &i->front() ), i->size() );
    }

    if( !seller.weapon.has_flag( "NO_UNWIELD" ) ) {
        check_item( item_location( seller, &seller.weapon ), 1 );
    }

    for( map_cursor &cursor : map_selector( seller.pos(), PICKUP_RANGE ) ) {
        buy_helper( cursor, check_item );
    }
    for( vehicle_cursor &cursor : vehicle_selector( seller.pos(), 1 ) ) {
        buy_helper( cursor, check_item );
    }

    return result;
}

void item_pricing::set_values( int ip_count )
{
    item *i_p = loc.get_item();
    is_container = i_p->is_container() || i_p->is_ammo_container();
    vol = i_p->volume();
    weight = i_p->weight();
    if( is_container || i_p->count() == 1 ) {
        count = ip_count;
    } else {
        charges = i_p->count();
        price /= charges;
        vol /= charges;
        weight /= charges;
    }
}

// Adjusts the pricing of an item, *unless* it is the currency of the
// faction we're trading with, as that should always be worth face value.
void item_pricing::adjust_values( const double adjust, faction *fac )
{
    if( !fac || fac->currency != loc.get_item()->typeId() ) {
        price *= adjust;
    }
}

void trading_window::setup_win( npc &np )
{
    w_head = catacurses::newwin( 4, TERMX, point_zero );
    w_them = catacurses::newwin( TERMY - 4, win_they_w, point( 0, 4 ) );
    w_you = catacurses::newwin( TERMY - 4, TERMX - win_they_w, point( win_they_w, 4 ) );
    mvwprintz( w_head, point_zero, c_white, header_message.c_str(), np.disp_name() );

    // Set up line drawings
    for( int i = 0; i < TERMX; i++ ) {
        mvwputch( w_head, point( i, 3 ), c_white, LINE_OXOX );
    }
    wrefresh( w_head );
    // End of line drawings
}

// 'cost' is the cost of a service the NPC may be rendering, if any.
void trading_window::setup_trade( int cost, npc &np )
{
    // Populate the list of what the NPC is willing to buy, and the prices they pay
    // Note that the NPC's barter skill is factored into these prices.
    // TODO: Recalc item values every time a new item is selected
    // Trading is not linear - starving NPC may pay $100 for 3 jerky, but not $100000 for 300 jerky
    theirs = npc_trading::init_buying( g->u, np, true );
    yours = npc_trading::init_buying( np, g->u, false );

    if( np.will_exchange_items_freely() ) {
        your_balance = 0;
    } else {
        your_balance = np.op_of_u.owed - cost;
    }
}

void trading_window::update_win( npc &np, const std::string &deal )
{
    if( update ) { // Time to re-draw
        update = false;
        // Draw borders, one of which is highlighted
        werase( w_them );
        werase( w_you );
        for( int i = 1; i < TERMX; i++ ) {
            mvwputch( w_head, point( i, 3 ), c_white, LINE_OXOX );
        }

        std::set<item *> without;
        std::vector<item *> added;

        for( item_pricing &pricing : yours ) {
            if( pricing.selected ) {
                added.push_back( pricing.loc.get_item() );
            }
        }

        for( item_pricing &pricing : theirs ) {
            if( pricing.selected ) {
                without.insert( pricing.loc.get_item() );
            }
        }

        bool npc_out_of_space = volume_left < 0_ml || weight_left < 0_gram;

        // Colors for hinting if the trade will be accepted or not.
        const nc_color trade_color       = npc_will_accept_trade( np ) ? c_green : c_red;
        const nc_color trade_color_light = npc_will_accept_trade( np ) ? c_light_green : c_light_red;

        mvwprintz( w_head, point( 2, 3 ),  npc_out_of_space ?  c_red : c_green,
                   _( "Volume: %s %s, Weight: %.1f %s" ),
                   format_volume( volume_left ), volume_units_abbr(),
                   convert_weight( weight_left ), weight_units() );

        std::string cost_str = _( "Exchange" );
        if( ! np.will_exchange_items_freely() ) {
            cost_str = string_format( your_balance >= 0 ? _( "Credit %s" ) : _( "Debt %s" ),
                                      format_money( std::abs( your_balance ) ) );
        }

        mvwprintz( w_head, point( TERMX / 2 + ( TERMX / 2 - cost_str.length() ) / 2, 3 ),
                   trade_color, cost_str );

        if( !deal.empty() ) {
            mvwprintz( w_head, point( ( TERMX - deal.length() ) / 2, 3 ),
                       trade_color_light, deal );
        }
        draw_border( w_them, ( focus_them ? c_yellow : BORDER_COLOR ) );
        draw_border( w_you, ( !focus_them ? c_yellow : BORDER_COLOR ) );

        mvwprintz( w_them, point( 2, 0 ), trade_color, np.name );
        mvwprintz( w_you,  point( 2, 0 ), trade_color, _( "You" ) );
#if defined(__ANDROID__)
        input_context ctxt( "NPC_TRADE" );
#endif
        // Draw lists of items, starting from offset
        for( size_t whose = 0; whose <= 1; whose++ ) {
            const bool they = whose == 0;
            const std::vector<item_pricing> &list = they ? theirs : yours;
            const size_t &offset = they ? them_off : you_off;
            const player &person = they ? static_cast<player &>( np ) :
                                   static_cast<player &>( g->u );
            catacurses::window &w_whose = they ? w_them : w_you;
            int win_w = getmaxx( w_whose );
            // Borders
            win_w -= 2;
            for( size_t i = offset; i < list.size() && i < entries_per_page + offset; i++ ) {
                const item_pricing &ip = list[i];
                const item *it = ip.loc.get_item();
                auto color = it == &person.weapon ? c_yellow : c_light_gray;
                const int &owner_sells = they ? ip.u_has : ip.npc_has;
                const int &owner_sells_charge = they ? ip.u_charges : ip.npc_charges;
                std::string itname = it->display_name();
                if( ip.loc.where() != item_location::type::character ) {
                    itname = itname + " " + ip.loc.describe( &g->u );
                    color = c_light_blue;
                }
                if( ip.charges > 0 && owner_sells_charge > 0 ) {
                    itname += string_format( _( ": trading %d" ), owner_sells_charge );
                } else {
                    if( ip.count > 1 ) {
                        itname += string_format( _( " (%d)" ), ip.count );
                    }
                    if( owner_sells ) {
                        itname += string_format( _( ": trading %d" ), owner_sells );
                    }
                }

                if( ip.selected ) {
                    color = c_white;
                }

                int keychar = i - offset + 'a';
                if( keychar > 'z' ) {
                    keychar = keychar - 'z' - 1 + 'A';
                }
                trim_and_print( w_whose, point( 1, i - offset + 1 ), win_w, color, "%c %c %s",
                                static_cast<char>( keychar ), ip.selected ? '+' : '-', itname );
#if defined(__ANDROID__)
                ctxt.register_manual_key( keychar, itname );
#endif

                std::string price_str = format_money( ip.price );
                nc_color price_color = np.will_exchange_items_freely() ? c_dark_gray : ( ip.selected ? c_white :
                                       c_light_gray );
                mvwprintz( w_whose, point( win_w - price_str.length(), i - offset + 1 ),
                           price_color, price_str );
            }
            if( offset > 0 ) {
                mvwprintw( w_whose, point( 1, entries_per_page + 2 ), _( "< Back" ) );
            }
            if( offset + entries_per_page < list.size() ) {
                mvwprintw( w_whose, point( 9, entries_per_page + 2 ), _( "More >" ) );
            }
        }
        wrefresh( w_head );
        wrefresh( w_them );
        wrefresh( w_you );
    } // Done updating the screen
}

void trading_window::show_item_data( npc &np, size_t offset,
                                     std::vector<item_pricing> &target_list )
{
    update = true;
    catacurses::window w_tmp = catacurses::newwin( 3, 21, point( 30 + ( TERMX - FULL_SCREEN_WIDTH ) / 2,
                               1 + ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w_tmp, point( 1, 1 ), c_red, _( "Examine which item?" ) );
    draw_border( w_tmp );
    wrefresh( w_tmp );
    // TODO: use input context
    size_t help = inp_mngr.get_input_event().get_first_input();
    if( help >= 'a' && help <= 'z' ) {
        help -= 'a';
    } else if( help >= 'A' && help <= 'Z' ) {
        help = help - 'A' + 26;
    } else {
        return;
    }

    mvwprintz( w_head, point_zero, c_white, header_message.c_str(), np.name );
    wrefresh( w_head );
    help += offset;
    if( help < target_list.size() ) {
        popup( target_list[help].loc.get_item()->info( true ), PF_NONE );
    }
}

int trading_window::get_var_trade( const item &it, int total_count )
{
    string_input_popup popup_input;
    int how_many = total_count;
    const std::string title = string_format( _( "Trade how many %s [MAX: %d]: " ),
                              it.display_name(), total_count );
    popup_input.title( title ).edit( how_many );
    if( popup_input.canceled() || how_many <= 0 ) {
        return -1;
    }
    return std::min( total_count, how_many );
}

bool trading_window::perform_trade( npc &np, const std::string &deal )
{
    size_t ch;

    volume_left = np.volume_capacity() - np.volume_carried();
    weight_left = np.weight_capacity() - np.weight_carried();

    // Shopkeeps are happy to have large inventories.
    if( np.mission == NPC_MISSION_SHOPKEEP ) {
        volume_left = 5'000'000_ml;
        weight_left = 5'000_kilogram;
    }

    do {
        update_win( np, deal );
#if defined(__ANDROID__)
        input_context ctxt( "NPC_TRADE" );
        ctxt.register_manual_key( '\t', "Switch lists" );
        ctxt.register_manual_key( '<', "Back" );
        ctxt.register_manual_key( '>', "More" );
        ctxt.register_manual_key( '?', "Examine item" );
#endif

        std::vector<item_pricing> &target_list = focus_them ? theirs : yours;
        size_t &offset = focus_them ? them_off : you_off;
        // TODO: use input context
        ch = inp_mngr.get_input_event().get_first_input();
        switch( ch ) {
            case '\t':
                focus_them = !focus_them;
                update = true;
                break;
            case '<':
                if( offset > 0 ) {
                    offset -= entries_per_page;
                    update = true;
                }
                break;
            case '>':
                if( offset + entries_per_page < target_list.size() ) {
                    offset += entries_per_page;
                    update = true;
                }
                break;
            case '?':
                show_item_data( np, offset, target_list );
                ch = ' ';
                break;
            case '\n':
                if( ! npc_will_accept_trade( np ) ) {

                    if( np.max_credit_extended() == 0 ) {
                        popup( _( "You'll need to offer me more than that." ) );
                    } else {
                        popup(
                            _( "Sorry, I'm only willing to extend you %s in credit." ),
                            format_money( np.max_credit_extended() )
                        );
                    }

                    update = true;
                    ch = ' ';
                } else if( volume_left < 0_ml || weight_left < 0_gram ) {
                    // Make sure NPC doesn't go over allowed volume
                    popup( _( "%s can't carry all that." ), np.name );
                    update = true;
                    ch = ' ';
                } else if( calc_npc_owes_you( np ) < your_balance ) {
                    // NPC is happy with the trade, but isn't willing to remember the whole debt.
                    const bool trade_ok = query_yn(
                                              _( "I'm never going to be able to pay you back for all that.  The most I'm willing to owe you is %s.\n\nContinue with trade?" ),
                                              format_money( np.max_willing_to_owe() )
                                          );

                    if( ! trade_ok ) {
                        update = true;
                        ch = ' ';
                    }
                } else {
                    if( ! query_yn( _( "Looks like a deal!  Accept this trade?" ) ) ) {
                        update = true;
                        ch = ' ';
                    }
                }
                break;
            default: // Letters & such
                if( ch >= 'a' && ch <= 'z' ) {
                    ch -= 'a';
                } else if( ch >= 'A' && ch <= 'Z' ) {
                    ch = ch - 'A' + ( 'z' - 'a' ) + 1;
                } else {
                    continue;
                }

                ch += offset;
                if( ch < target_list.size() ) {
                    item_pricing &ip = target_list[ch];
                    int change_amount = 1;
                    int &owner_sells = focus_them ? ip.u_has : ip.npc_has;
                    int &owner_sells_charge = focus_them ? ip.u_charges : ip.npc_charges;

                    if( ip.selected ) {
                        if( owner_sells_charge > 0 ) {
                            change_amount = owner_sells_charge;
                            owner_sells_charge = 0;
                        } else if( owner_sells > 0 ) {
                            change_amount = owner_sells;
                            owner_sells = 0;
                        }
                    } else if( ip.charges > 0 ) {
                        change_amount = get_var_trade( *ip.loc.get_item(), ip.charges );
                        if( change_amount < 1 ) {
                            ch = 0;
                            continue;
                        }
                        owner_sells_charge = change_amount;
                    } else {
                        if( ip.count > 1 ) {
                            change_amount = get_var_trade( *ip.loc.get_item(), ip.count );
                            if( change_amount < 1 ) {
                                ch = 0;
                                continue;
                            }
                        }
                        owner_sells = change_amount;
                    }
                    update = true;
                    ip.selected = !ip.selected;
                    if( ip.selected != focus_them ) {
                        change_amount *= -1;
                    }
                    int delta_price = ip.price * change_amount;
                    if( !np.will_exchange_items_freely() ) {
                        your_balance -= delta_price;
                    }
                    if( ip.loc.where() == item_location::type::character ) {
                        volume_left += ip.vol * change_amount;
                        weight_left += ip.weight * change_amount;
                    }
                }
                ch = 0;
        }
    } while( ch != KEY_ESCAPE && ch != '\n' );

    return ch == '\n';
}

// Returns how much the NPC will owe you after this transaction.
// You must also check if they will accept the trade.
int trading_window::calc_npc_owes_you( const npc &np ) const
{
    // Friends don't hold debts against friends.
    if( np.will_exchange_items_freely() ) {
        return 0;
    }

    // If they're going to owe you more than before, and it's more than they're willing
    // to owe, then cap the amount owed at the present level or their willingness to owe
    // (whichever is bigger).
    //
    // When could they owe you more than max_willing_to_owe? It could be from quest rewards,
    // when they were less angry, or from when you were better friends.
    if( your_balance > np.op_of_u.owed && your_balance > np.max_willing_to_owe() ) {
        return std::max( np.op_of_u.owed, np.max_willing_to_owe() );
    }

    // Fair's fair. NPC will remember this debt (or credit they've extended)
    return your_balance;
}

void trading_window::update_npc_owed( npc &np )
{
    np.op_of_u.owed = calc_npc_owes_you( np );
}

// Oh my aching head
// op_of_u.owed is the positive when the NPC owes the player, and negative if the player owes the
// NPC
// cost is positive when the player owes the NPC money for a service to be performed
bool npc_trading::trade( npc &np, int cost, const std::string &deal )
{
    np.shop_restock();
    //np.drop_items( np.weight_carried() - np.weight_capacity(),
    //               np.volume_carried() - np.volume_capacity() );
    np.drop_invalid_inventory();

    trading_window trade_win;
    trade_win.setup_win( np );
    trade_win.setup_trade( cost, np );

    bool traded = trade_win.perform_trade( np, deal );
    if( traded ) {
        int practice = 0;

        std::list<item_location *> from_map;

        npc_trading::transfer_items( trade_win.yours, g->u, np, from_map, false );
        npc_trading::transfer_items( trade_win.theirs, np, g->u, from_map, true );

        for( item_location *loc_ptr : from_map ) {
            loc_ptr->remove_item();
        }

        // NPCs will remember debts, to the limit that they'll extend credit or previous debts
        if( ! np.will_exchange_items_freely() ) {
            trade_win.update_npc_owed( np );
            g->u.practice( skill_barter, practice / 10000 );
        }
    }
    g->refresh_all();
    return traded;
}

// Will the NPC accept the trade that's currently on offer?
bool trading_window::npc_will_accept_trade( const npc &np ) const
{
    return np.is_player_ally() || your_balance + np.max_credit_extended() > 0;
}
