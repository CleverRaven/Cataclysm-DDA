#pragma once
#ifndef CATA_SRC_NPCTRADE_H
#define CATA_SRC_NPCTRADE_H

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "item_location.h"
#include "trade_ui.h"
#include "units.h"

constexpr char const *VAR_TRADE_IGNORE = "trade_ignore";

class Character;
class item;
class npc;

class item_pricing
{
    public:
        item_pricing( Character &c, item &it, double v, int count ) : loc( c, &it ), price( v ) {
            set_values( count );
        }

        item_pricing( item_location &&l, double v, int count ) : loc( std::move( l ) ), price( v ) {
            set_values( count );
        }
        void set_values( int ip_count );

        item_location loc;
        double price = 0;
        // Whether this is selected for trading
        bool selected = false;
        bool is_container = false;
        int count = 0;
        int charges = 0;
        int u_has = 0;
        int npc_has = 0;
        int u_charges = 0;
        int npc_charges = 0;
        units::mass weight = 0_gram;
        units::volume vol = 0_ml;
};
namespace npc_trading
{
bool pay_npc( npc &np, int cost );

int bionic_install_price( Character &installer, Character &patient, item_location const &bionic );
int adjusted_price( item const *it, int amount, Character const &buyer, Character const &seller );
int trading_price( Character const &buyer, Character const &seller,
                   trade_selector::entry_t const &it );
int calc_npc_owes_you( const npc &np, int your_balance );
bool npc_will_accept_trade( npc const &np, int your_balance );
bool npc_can_fit_items( npc const &np, trade_selector::select_t const &to_trade );
void update_npc_owed( npc &np, int your_balance, int your_sale_value );
int cash_to_favor( const npc &, int cash );

std::list<item> transfer_items( trade_selector::select_t &stuff, Character &giver,
                                Character &receiver, std::list<item_location *> &from_map,
                                bool use_escrow );
double net_price_adjustment( const Character &buyer, const Character &seller );
bool trade( npc &p, int cost, const std::string &deal );
std::vector<item_pricing> init_selling( npc &p );
} // namespace npc_trading

#endif // CATA_SRC_NPCTRADE_H
