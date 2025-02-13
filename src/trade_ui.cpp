
#include "trade_ui.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <memory>
#include <unordered_set>

#include "character.h"
#include "clzones.h"
#include "color.h"
#include "enums.h"
#include "game_constants.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_category.h"
#include "npc.h"
#include "npc_opinion.h"
#include "npctrade.h"
#include "npctrade_utils.h"
#include "options.h"
#include "output.h"
#include "point.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "type_id.h"

static const flag_id json_flag_NO_UNWIELD( "NO_UNWIELD" );
static const item_category_id item_category_ITEMS_WORN( "ITEMS_WORN" );
static const item_category_id item_category_WEAPON_HELD( "WEAPON_HELD" );

namespace
{
point _pane_orig( int side )
{
    return { side > 0 ? TERMX / 2 : 0, trade_ui::header_size };
}

point _pane_size()
{
    return { TERMX / 2, TERMY - _pane_orig( 0 ).y };
}

} // namespace

trade_preset::trade_preset( Character const &you, Character const &trader )
    : _u( you ), _trader( trader )
{
    save_state = &inventory_ui_default_state;
    append_cell(
    [&]( item_location const & loc ) {
        return format_money( npc_trading::trading_price( _trader, _u, { loc, 1 } ) );
    },
    _( "Unit price" ) );
}

bool trade_preset::is_shown( item_location const &loc ) const
{
    return !loc->has_var( VAR_TRADE_IGNORE ) && inventory_selector_preset::is_shown( loc ) &&
           loc->is_owned_by( _u ) && loc->made_of( phase_id::SOLID ) && !loc->is_frozen_liquid() &&
           ( !_u.is_wielding( *loc ) || !loc->has_flag( json_flag_NO_UNWIELD ) );
}

std::string trade_preset::get_denial( const item_location &loc ) const
{
    int const price = npc_trading::trading_price( _trader, _u, { loc, 1 } );

    if( _u.is_npc() ) {
        npc const &np = *_u.as_npc();
        ret_val<void> const ret = np.wants_to_sell( loc, price );
        if( !ret.success() ) {
            if( ret.str().empty() ) {
                return string_format( _( "%s does not want to sell this" ), np.get_name() );
            }
            return np.replace_with_npc_name( ret.str() );
        }
    } else if( _trader.is_npc() ) {
        npc const &np = *_trader.as_npc();
        ret_val<void> const ret = np.wants_to_buy( *loc, price );
        if( !ret.success() ) {
            if( ret.str().empty() ) {
                return string_format( _( "%s does not want to buy this" ), np.get_name() );
            }
            return np.replace_with_npc_name( ret.str() );
        }
    }

    if( _u.is_worn( *loc ) ) {
        ret_val<void> const ret = const_cast<Character &>( _u ).can_takeoff( *loc );
        if( !ret.success() ) {
            return _u.replace_with_npc_name( ret.str() );
        }
    }

    return inventory_selector_preset::get_denial( loc );
}

bool trade_preset::cat_sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const
{
    // sort worn and held categories last we likely don't want to trade them
    auto const fudge_rank = []( inventory_entry const & e ) -> int {
        item_category_id const cat = e.get_category_ptr()->get_id();
        int const rank = e.get_category_ptr()->sort_rank();
        return cat != item_category_ITEMS_WORN && cat != item_category_WEAPON_HELD ? rank : rank + 10000;
    };
    return fudge_rank( lhs ) < fudge_rank( rhs );
}

trade_ui::trade_ui( party_t &you, npc &trader, currency_t cost, std::string title )
    : _upreset{ you, trader }, _tpreset{ trader, you },
      _panes{ std::make_unique<pane_t>( this, trader, _tpreset, std::string(), _pane_size(),
                                        _pane_orig( -1 ) ),
              std::make_unique<pane_t>( this, you, _upreset, std::string(), _pane_size(),
                                        _pane_orig( 1 ) ) },
      _parties{ &trader, &you }, _title( std::move( title ) )

{
    _panes[_you]->add_character_items( you );
    _panes[_you]->add_nearby_items( 1 );
    _panes[_trader]->add_character_items( trader );
    if( trader.is_shopkeeper() ) {
        _panes[_trader]->categorize_map_items( true );

        add_fallback_zone( trader );

        zone_manager &zmgr = zone_manager::get_manager();

        std::unordered_set<tripoint_bub_ms> const src =
            zmgr.get_point_set_loot( trader.pos_abs(), PICKUP_RANGE, trader.get_fac_id() );

        for( tripoint_bub_ms const &pt : src ) {
            _panes[_trader]->add_map_items( pt );
            _panes[_trader]->add_vehicle_items( pt );
        }
    } else if( !trader.is_player_ally() ) {
        _panes[_trader]->add_nearby_items( 1 );
    }

    if( trader.will_exchange_items_freely() ) {
        _cost = 0;
    } else {
        _cost = trader.op_of_u.owed - cost;
    }
    _balance = _cost;
    _bank = you.cash;
    _delta_bank = 0;
    _panes[_you]->get_active_column().on_deactivate();

    _header_ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        _header_w = catacurses::newwin( header_size, TERMX, point::zero );
        ui.position_from_window( _header_w );
        ui.invalidate_ui();
        resize();
    } );
    _header_ui.mark_resize();
    _header_ui.on_redraw( [this]( ui_adaptor const & /* ui */ ) {
        werase( _header_w );
        _draw_header();
        wnoutrefresh( _header_w );
    } );
}

void trade_ui::pushevent( event const &ev )
{
    _queue.emplace( ev );
}

trade_ui::trade_result_t trade_ui::perform_trade()
{
    _exit = false;
    _traded = false;

    while( !_exit ) {
        _panes[_cpane]->execute();

        while( !_queue.empty() ) {
            event const ev = _queue.front();
            _queue.pop();
            _process( ev );
        }
    }

    if( _traded ) {
        return { _traded,
                 _balance,
                 _delta_bank,
                 _trade_values[_you],
                 _trade_values[_trader],
                 _panes[_you]->to_trade(),
                 _panes[_trader]->to_trade() };
    }

    return { false, 0, 0, 0, 0, {}, {} };
}

void trade_ui::recalc_values_cpane()
{
    _trade_values[_cpane] = 0;

    for( entry_t const &it : _panes[_cpane]->to_trade() ) {
        // FIXME: cache trading_price
        _trade_values[_cpane] +=
            npc_trading::trading_price( *_parties[-_cpane + 1], *_parties[_cpane], it );
    }
    if( !_parties[_trader]->as_npc()->will_exchange_items_freely() ) {
        _balance = _cost + _trade_values[_you] - _trade_values[_trader] + _delta_bank;
    }
    _header_ui.invalidate_ui();
}

void trade_ui::autobalance()
{
    int const sign = _cpane == _you ? -1 : 1;
    if( ( sign < 0 && _balance < 0 ) || ( sign > 0 && _balance > 0 ) ) {
        inventory_entry &entry = _panes[_cpane]->get_active_column().get_highlighted();
        size_t const avail = entry.get_available_count() - entry.chosen_count;
        double const price = npc_trading::trading_price( *_parties[-_cpane + 1], *_parties[_cpane],
                             entry_t{ entry.any_item(), 1 } ) * sign;
        double const num = _balance / price;
        double const extra = sign < 0 ? std::ceil( num ) : std::floor( num );
        _panes[_cpane]->toggle_entry( entry, entry.chosen_count +
                                      std::min( static_cast<size_t>( extra ), avail ) );
    }
}

void trade_ui::bank_balance()
{
    if( !get_option<bool>( "CAPITALISM" ) ) {
        popup( _( "Your promises of digital payment mean nothing here." ) );
        return;
    }
    _bank += _delta_bank;
    _delta_bank = -( _balance - _delta_bank );
    if( _delta_bank > 0 ) { // a withdrawal
        _delta_bank = std::min( _delta_bank, _bank );
        _delta_bank = std::max( _delta_bank, 0 );
    }
    _bank -= _delta_bank;
    recalc_values_cpane();
}

void trade_ui::resize()
{
    _panes[_you]->resize( _pane_size(), _pane_orig( 1 ) );
    _panes[_trader]->resize( _pane_size(), _pane_orig( -1 ) );
}

void trade_ui::_process( event const &ev )
{
    switch( ev ) {
        case event::TRADECANCEL: {
            _traded = false;
            _exit = true;
            break;
        }
        case event::TRADEOK: {
            _traded = _confirm_trade();
            _exit = _traded;
            break;
        }
        case event::SWITCH: {
            _panes[_cpane]->get_ui()->invalidate_ui();
            _cpane = -_cpane + 1;
            break;
        }
        case event::NEVENTS: {
            break;
        }
    }
}

bool trade_ui::_confirm_trade() const
{
    npc const &np = *_parties[_trader]->as_npc();

    if( !npc_trading::npc_will_accept_trade( np, _balance ) ) {
        if( np.max_credit_extended() == 0 ) {
            popup( _( "You'll need to offer me more than that." ) );
        } else {
            popup( _( "Sorry, I'm only willing to extend you %s in credit." ),
                   format_money( np.max_credit_extended() ) );
        }
    } else if( !np.is_shopkeeper() &&
               !npc_trading::npc_can_fit_items( np, _panes[_you]->to_trade() ) ) {
        popup( _( "%s doesn't have the appropriate pockets to accept that." ), np.get_name() );
    } else if( npc_trading::calc_npc_owes_you( np, _balance ) < _balance ) {
        // NPC is happy with the trade, but isn't willing to remember the whole debt.
        return query_yn(
                   _( "I'm never going to be able to pay you back for all that.  The most I'm "
                      "willing to owe you is %s.\n\nContinue with trade?" ),
                   format_money( np.max_willing_to_owe() ) );

    } else {
        return query_yn( _( "Looks like a deal!  Accept this trade?" ) );
    }

    return false;
}

void trade_ui::_draw_header()
{
    draw_border( _header_w, c_light_gray );
    center_print( _header_w, 1, c_white, _title );
    npc const &np = *_parties[_trader]->as_npc();
    nc_color const trade_color =
        npc_trading::npc_will_accept_trade( np, _balance ) ? c_green : c_red;
    std::string cost_str = _( "Exchange" );
    if( !np.will_exchange_items_freely() ) {
        cost_str = string_format( _balance >= 0 ? _( "Credit %s" ) : _( "Debt %s" ),
                                  format_money( std::abs( _balance ) ) );
    }
    center_print( _header_w, 2, trade_color, cost_str );
    mvwprintz( _header_w, { 1, 3 }, c_white, _parties[_trader]->get_name() );
    right_print( _header_w, 3, 1, c_white, _( "You" ) );
    center_print( _header_w, header_size - 1, c_white,
                  string_format( _( "%s to switch panes" ),
                                 colorize( _panes[_you]->get_ctxt()->get_desc(
                                         trade_selector::ACTION_SWITCH_PANES ),
                                           c_yellow ) ) );
    center_print( _header_w, header_size - 2, c_white,
                  string_format( _( "%s to auto balance with highlighted item" ),
                                 colorize( _panes[_you]->get_ctxt()->get_desc(
                                         trade_selector::ACTION_AUTOBALANCE ),
                                           c_yellow ) ) );
    if( get_option<bool>( "CAPITALISM" ) ) {
        right_print( _header_w, 2, 1, c_white, string_format( _( "Bank Balance: %s" ),
                     format_money( _bank ) ) );
        right_print( _header_w, 1, 1, c_white,
                     string_format( _( "%s to balance trade with bank account balance" ),
                                    colorize( _panes[_you]->get_ctxt()->get_desc(
                                            trade_selector::ACTION_BANKBALANCE ),
                                              c_yellow ) ) );
    }

}
