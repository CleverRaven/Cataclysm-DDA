
#include "trade_ui.h"

#include <cmath>
#include <cstdlib>
#include <memory>

#include "character.h"
#include "color.h"
#include "enums.h"
#include "game_constants.h"
#include "inventory_ui.h"
#include "item.h"
#include "npc.h"
#include "npctrade.h"
#include "output.h"
#include "point.h"
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
    append_cell(
    [&]( item_location const & loc ) {
        return format_money( npc_trading::trading_price( _trader, _u, { loc, 1 } ) );
    },
    _( "Unit price" ) );
}

bool trade_preset::is_shown( item_location const &loc ) const
{
    return inventory_selector_preset::is_shown( loc ) and loc->is_owned_by( _u ) and
           loc->made_of( phase_id::SOLID ) and
           ( !_u.is_wielding( *loc ) or !loc->has_flag( json_flag_NO_UNWIELD ) );
}

std::string trade_preset::get_denial( const item_location &loc ) const
{
    int const price = npc_trading::trading_price( _trader, _u, { loc, 1 } );
    int const market_price = loc->price( true );

    if( _u.is_npc() ) {
        npc const &np = *_u.as_npc();
        if( !np.wants_to_sell( *loc, price, market_price ) ) {
            return string_format( _( "%s does not want to sell this" ), np.get_name() );
        }
    } else if( _trader.is_npc() ) {
        npc const &np = *_trader.as_npc();
        if( !np.wants_to_buy( *loc, price, market_price ) ) {
            return string_format( _( "%s does not want to buy this" ), np.get_name() );
        }
    }

    return inventory_selector_preset::get_denial( loc );
}

bool trade_preset::cat_sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const
{
    item_category const *const lcat = lhs.get_category_ptr();
    if( lcat->get_id() == item_category_ITEMS_WORN or lcat->get_id() == item_category_WEAPON_HELD ) {
        return false;
    }
    item_category const *const rcat = rhs.get_category_ptr();
    if( rcat->get_id() == item_category_ITEMS_WORN or rcat->get_id() == item_category_WEAPON_HELD ) {
        return true;
    }

    return inventory_selector_preset::cat_sort_compare( lhs, rhs );
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
    if( trader.mission == NPC_MISSION_SHOPKEEP ) {
        _panes[_trader]->categorize_map_items( true );
        _panes[_trader]->add_nearby_items( PICKUP_RANGE );
    } else if( !trader.is_player_ally() ) {
        _panes[_trader]->add_nearby_items( 1 );
    }

    if( trader.will_exchange_items_freely() ) {
        _cost = 0;
    } else {
        _cost = trader.op_of_u.owed - cost;
    }
    _balance = _cost;

    _panes[_you]->get_active_column().on_deactivate();

    _header_ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        _header_w = catacurses::newwin( header_size, TERMX, point_zero );
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
                 _trade_values[_you],
                 _trade_values[_trader],
                 _panes[_you]->to_trade(),
                 _panes[_trader]->to_trade() };
    }

    return { false, 0, 0, 0, {}, {} };
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
        _balance = _cost + _trade_values[_you] - _trade_values[_trader];
    }
    _header_ui.invalidate_ui();
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
    } else if( np.mission != NPC_MISSION_SHOPKEEP &&
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
}
