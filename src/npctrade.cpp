#include "npctrade.h"

#include <algorithm>
#include <cstdlib>
#include <list>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "faction.h"
#include "game.h"
#include "game_constants.h"
#include "input.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "map_selector.h"
#include "npc.h"
#include "optional.h"
#include "output.h"
#include "player.h"
#include "point.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "units.h"
#include "vehicle_selector.h"
#include "visitable.h"

static const skill_id skill_barter( "barter" );

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
            if( ip.charges > 0 ) {
                giver.use_charges( gift.typeId(), charges );
            } else if( ip.count > 0 ) {
                for( int i = 0; i < count; i++ ) {
                    giver.use_amount( gift.typeId(), 1 );
                }
            }
        } else {
            if( ip.charges > 0 ) {
                ip.loc.get_item()->set_var( "trade_charges", charges );
            } else {
                ip.loc.get_item()->set_var( "trade_amount", 1 );
            }
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
            result.emplace_back( np, i->front(), val, static_cast<int>( i->size() ) );
        }
    }

    if(
        np.will_exchange_items_freely() &&
        !np.weapon.is_null() &&
        !np.weapon.has_flag( "NO_UNWIELD" )
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

    const auto check_item = [fac, adjust, is_npc, &np, &result, &seller]( item_location
    loc, int count = 1 ) {
        if( !loc ) {
            return;
        }
        item &it = *loc;

        // Don't sell items we don't own.
        if( !it.is_owned_by( seller ) ) {
            return;
        }

        if( it.count() <= 0 ) {
            debugmsg( "item %s has zero or negative charges", it.typeId().str() );
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

    for( item_location loc : seller.all_items_loc() ) {
        if( seller.is_wielding( *loc ) && loc->has_flag( "NO_UNWIELD" ) ) {
            continue;
        }
        check_item( loc, loc->count() );
    }

    //nearby items owned by the NPC will only show up in
    //the trade window if the NPC is also a shopkeeper
    if( np.mission == NPC_MISSION_SHOPKEEP ) {
        for( map_cursor &cursor : map_selector( seller.pos(), PICKUP_RANGE ) ) {
            buy_helper( cursor, check_item );
        }
    }

    for( vehicle_cursor &cursor : vehicle_selector( seller.pos(), 1 ) ) {
        buy_helper( cursor, check_item );
    }

    const auto cmp = []( const item_pricing & a, const item_pricing & b ) {
        // Sort items by category first, then name.
        return localized_compare( std::make_pair( a.loc->get_category(), a.loc->display_name() ),
                                  std::make_pair( b.loc->get_category(), b.loc->display_name() ) );
    };

    std::sort( result.begin(), result.end(), cmp );

    return result;
}

void item_pricing::set_values( int ip_count )
{
    const item *i_p = loc.get_item();
    is_container = i_p->is_container() || i_p->is_ammo_container();
    vol = i_p->volume();
    weight = i_p->weight();
    if( is_container || i_p->count() == 1 ) {
        count = ip_count;
    } else {
        charges = i_p->count();
        if( charges > 0 ) {
            price /= charges;
            vol /= charges;
            weight /= charges;
        } else {
            debugmsg( "item %s has zero or negative charges", i_p->typeId().str() );
        }
    }
}

// Adjusts the pricing of an item, *unless* it is the currency of the
// faction we're trading with, as that should always be worth face value.
void item_pricing::adjust_values( const double adjust, const faction *fac )
{
    if( !fac || fac->currency != loc.get_item()->typeId() ) {
        price *= adjust;
    }
}

void trading_window::setup_win( ui_adaptor &ui )
{
    const int win_they_w = TERMX / 2;
    entries_per_page = std::min( TERMY - 7, 2 + ( 'z' - 'a' ) + ( 'Z' - 'A' ) );
    w_head = catacurses::newwin( 4, TERMX, point_zero );
    w_them = catacurses::newwin( TERMY - 4, win_they_w, point( 0, 4 ) );
    w_you = catacurses::newwin( TERMY - 4, TERMX - win_they_w, point( win_they_w, 4 ) );
    ui.position( point_zero, point( TERMX, TERMY ) );
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
    // Draw borders, one of which is highlighted
    werase( w_them );
    werase( w_you );

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
    const nc_color trade_color = npc_will_accept_trade( np ) ? c_green : c_red;

    input_context ctxt( "NPC_TRADE" );

    werase( w_head );
    fold_and_print( w_head, point_zero, getmaxx( w_head ), c_white,
                    _( "Trading with %s.\n"
                       "%s to switch lists, letters to pick items, "
                       "%s to finalize, %s to quit, "
                       "%s to get information on an item." ),
                    np.disp_name(),
                    ctxt.get_desc( "SWITCH_LISTS" ),
                    ctxt.get_desc( "CONFIRM" ),
                    ctxt.get_desc( "QUIT" ),
                    ctxt.get_desc( "EXAMINE" ) );

    // Set up line drawings
    for( int i = 0; i < TERMX; i++ ) {
        mvwputch( w_head, point( i, 3 ), c_white, LINE_OXOX );
    }
    // End of line drawings

    mvwprintz( w_head, point( 2, 3 ),  npc_out_of_space ?  c_red : c_green,
               _( "Volume: %s %s, Weight: %.1f %s" ),
               format_volume( volume_left ), volume_units_abbr(),
               convert_weight( weight_left ), weight_units() );

    std::string cost_str = _( "Exchange" );
    if( !np.will_exchange_items_freely() ) {
        cost_str = string_format( your_balance >= 0 ? _( "Credit %s" ) : _( "Debt %s" ),
                                  format_money( std::abs( your_balance ) ) );
    }

    mvwprintz( w_head, point( TERMX / 2 + ( TERMX / 2 - utf8_width( cost_str ) ) / 2, 3 ),
               trade_color, cost_str );

    if( !deal.empty() ) {
        const nc_color trade_color_light = npc_will_accept_trade( np ) ? c_light_green : c_light_red;
        mvwprintz( w_head, point( ( TERMX - utf8_width( deal ) ) / 2, 3 ),
                   trade_color_light, deal );
    }
    draw_border( w_them, ( focus_them ? c_yellow : BORDER_COLOR ) );
    draw_border( w_you, ( !focus_them ? c_yellow : BORDER_COLOR ) );

    mvwprintz( w_them, point( 2, 0 ), trade_color, np.name );
    mvwprintz( w_you,  point( 2, 0 ), trade_color, _( "You" ) );
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

            if( np.will_exchange_items_freely() && ip.loc.where() != item_location::type::character ) {
                itname = itname + " (" + ip.loc.describe( &g->u ) + ")";
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
            mvwprintz( w_whose, point( win_w - utf8_width( price_str ), i - offset + 1 ),
                       price_color, price_str );
        }
        if( offset > 0 ) {
            mvwprintw( w_whose, point( 1, entries_per_page + 2 ), _( "< Back" ) );
        }
        if( offset + entries_per_page < list.size() ) {
            mvwprintw( w_whose, point( 9, entries_per_page + 2 ), _( "More >" ) );
        }
    }
    wnoutrefresh( w_head );
    wnoutrefresh( w_them );
    wnoutrefresh( w_you );
}

void trading_window::show_item_data( size_t offset,
                                     std::vector<item_pricing> &target_list )
{
    catacurses::window w_tmp;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        w_tmp = catacurses::newwin( 3, 21,
                                    point( 30 + ( TERMX - FULL_SCREEN_WIDTH ) / 2,
                                           1 + ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ) );
        ui.position_from_window( w_tmp );
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_tmp );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w_tmp, point( 1, 1 ), c_red, _( "Examine which item?" ) );
        draw_border( w_tmp );
        wnoutrefresh( w_tmp );
    } );

    input_context ctxt( "NPC_TRADE" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    bool exit = false;
    while( !exit ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "QUIT" ) {
            exit = true;
        } else if( action == "ANY_INPUT" ) {
            const input_event evt = ctxt.get_raw_input();
            if( evt.type != input_event_t::keyboard || evt.sequence.empty() ) {
                continue;
            }
            size_t help = evt.get_first_input();
            if( help >= 'a' && help <= 'z' ) {
                help -= 'a';
            } else if( help >= 'A' && help <= 'Z' ) {
                help = help - 'A' + 26;
            } else {
                continue;
            }

            help += offset;
            if( help < target_list.size() ) {
                std::vector<iteminfo> info;
                const item &itm = *target_list[help].loc.get_item();
                itm.info( true, info );
                item_info_data data( itm.tname(),
                                     itm.type_name(),
                                     info, {} );
                data.handle_scrolling = true;
                data.any_input = false;
                draw_item_info( []() -> catacurses::window {
                    const int width = std::min( TERMX, FULL_SCREEN_WIDTH );
                    const int height = std::min( TERMY, FULL_SCREEN_HEIGHT );
                    return catacurses::newwin( height, width, point( ( TERMX - width ) / 2, ( TERMY - height ) / 2 ) );
                }, data );
                exit = true;
            }
        }
    }
}

int trading_window::get_var_trade( const item &it, int total_count )
{
    string_input_popup popup_input;
    int how_many = total_count;

    const std::string title = string_format( _( "Trade how many %s [MAX: %d]: " ), it.tname( how_many ),
                              total_count );
    popup_input.title( title ).edit( how_many );
    if( popup_input.canceled() || how_many <= 0 ) {
        return -1;
    }
    return std::min( total_count, how_many );
}

bool trading_window::perform_trade( npc &np, const std::string &deal )
{
    weight_left = np.weight_capacity() - np.weight_carried();

    // Shopkeeps are happy to have large inventories.
    if( np.mission == NPC_MISSION_SHOPKEEP ) {
        volume_left = 5'000_liter;
        weight_left = 5'000_kilogram;
    }

    input_context ctxt( "NPC_TRADE" );
    ctxt.register_action( "SWITCH_LISTS" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "ANY_INPUT" );

    ui_adaptor ui;
    ui.on_screen_resize( [this]( ui_adaptor & ui ) {
        setup_win( ui );
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        update_win( np, deal );
    } );

    bool confirm = false;
    bool exit = false;
    while( !exit ) {
        ui_manager::redraw();

        std::vector<item_pricing> &target_list = focus_them ? theirs : yours;
        size_t &offset = focus_them ? them_off : you_off;
        const std::string action = ctxt.handle_input();
        if( action == "SWITCH_LISTS" ) {
            focus_them = !focus_them;
        } else if( action == "PAGE_UP" ) {
            if( offset > entries_per_page ) {
                offset -= entries_per_page;
            } else {
                offset = 0;
            }
        } else if( action == "PAGE_DOWN" ) {
            if( offset + entries_per_page < target_list.size() ) {
                offset += entries_per_page;
            }
        } else if( action == "EXAMINE" ) {
            show_item_data( offset, target_list );
        } else if( action == "CONFIRM" ) {
            if( !npc_will_accept_trade( np ) ) {

                if( np.max_credit_extended() == 0 ) {
                    popup( _( "You'll need to offer me more than that." ) );
                } else {
                    popup(
                        _( "Sorry, I'm only willing to extend you %s in credit." ),
                        format_money( np.max_credit_extended() )
                    );
                }
            } else if( volume_left < 0_ml || weight_left < 0_gram ) {
                // Make sure NPC doesn't go over allowed volume
                popup( _( "%s can't carry all that." ), np.name );
            } else if( calc_npc_owes_you( np ) < your_balance ) {
                // NPC is happy with the trade, but isn't willing to remember the whole debt.
                const bool trade_ok = query_yn(
                                          _( "I'm never going to be able to pay you back for all that.  The most I'm willing to owe you is %s.\n\nContinue with trade?" ),
                                          format_money( np.max_willing_to_owe() )
                                      );

                if( trade_ok ) {
                    exit = true;
                    confirm = true;
                }
            } else {
                if( query_yn( _( "Looks like a deal!  Accept this trade?" ) ) ) {
                    exit = true;
                    confirm = true;
                }
            }
        } else if( action == "QUIT" ) {
            exit = true;
            confirm = false;
        } else if( action == "ANY_INPUT" ) {
            const input_event evt = ctxt.get_raw_input();
            if( evt.type != input_event_t::keyboard || evt.sequence.empty() ) {
                continue;
            }
            size_t ch = evt.get_first_input();
            // Letters & such
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
                        continue;
                    }
                    owner_sells_charge = change_amount;
                } else {
                    if( ip.count > 1 ) {
                        change_amount = get_var_trade( *ip.loc.get_item(), ip.count );
                        if( change_amount < 1 ) {
                            continue;
                        }
                    }
                    owner_sells = change_amount;
                }
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
        }
    }

    return confirm;
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
    trade_win.setup_trade( cost, np );

    bool traded = trade_win.perform_trade( np, deal );
    if( traded ) {
        int practice = 0;

        std::list<item_location *> from_map;

        npc_trading::transfer_items( trade_win.yours, g->u, np, from_map, false );
        npc_trading::transfer_items( trade_win.theirs, np, g->u, from_map, true );

        for( item_location *loc_ptr : from_map ) {
            if( !loc_ptr ) {
                continue;
            }
            item *it = loc_ptr->get_item();
            if( !it ) {
                continue;
            }
            if( it->has_var( "trade_charges" ) && it->count_by_charges() ) {
                it->charges -= static_cast<int>( it->get_var( "trade_charges", 0 ) );
                if( it->charges <= 0 ) {
                    loc_ptr->remove_item();
                } else {
                    it->erase_var( "trade_charges" );
                }
            } else if( it->has_var( "trade_amount" ) ) {
                loc_ptr->remove_item();
            }
        }

        // NPCs will remember debts, to the limit that they'll extend credit or previous debts
        if( !np.will_exchange_items_freely() ) {
            trade_win.update_npc_owed( np );
            g->u.practice( skill_barter, practice / 10000 );
        }
    }
    return traded;
}

// Will the NPC accept the trade that's currently on offer?
bool trading_window::npc_will_accept_trade( const npc &np ) const
{
    return np.will_exchange_items_freely() || your_balance + np.max_credit_extended() > 0;
}
