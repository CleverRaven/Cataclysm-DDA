#pragma once
#ifndef NPCTRADE_H
#define NPCTRADE_H

#include <stddef.h>
#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <list>

#include "inventory.h"
#include "item_location.h"
#include "output.h"
#include "units.h"
#include "cursesdef.h"
#include "translations.h"

class Character;
class faction;
class item;
class npc;
class player;

class item_pricing
{
    public:
        item_pricing( Character &c, item &it, int v, int count ) : loc( c, &it ), price( v ) {
            set_values( count );
        }

        item_pricing( item_location &&l, int v, int count ) : loc( std::move( l ) ), price( v ) {
            set_values( count );
        }
        void set_values( int ip_count );
        void adjust_values( const double adjust, faction *fac );

        item_location loc;
        int price;
        // Whether this is selected for trading
        bool selected = false;
        bool is_container;
        int count = 0;
        int charges = 0;
        int u_has = 0;
        int npc_has = 0;
        int u_charges = 0;
        int npc_charges = 0;
        units::mass weight = 0_gram;
        units::volume vol = 0_ml;
};

class trading_window
{
    public:
        trading_window() = default;
        std::vector<item_pricing> theirs;
        std::vector<item_pricing> yours;
        bool exchange;
        int u_get;
        int npc_requires;

        void setup_win( npc &np );
        void setup_trade( int cost, npc &np );
        void update_win( npc &p, const std::string &deal, const int adjusted_u_get );
        void show_item_data( npc &np, size_t offset, std::vector<item_pricing> &target_list );
        bool perform_trade( npc &p, const std::string &deal );
        void update_npc_owed( npc &np );

    private:
        catacurses::window w_head;
        catacurses::window w_them;
        catacurses::window w_you;
        const int win_they_w = TERMX / 2;
        const std::string header_message = _( "TAB key to switch lists, letters to pick items,"
                                              "Enter to finalize, Esc to quit,\n"
                                              "? to get information on an item." );
        const size_t entries_per_page = std::min( TERMY - 7, 2 + ( 'z' - 'a' ) + ( 'Z' - 'A' ) );
        bool update = true;
        bool focus_them = true; // Is the focus on them?
        size_t them_off = 0, you_off = 0; // Offset from the start of the list

        inventory temp;
        units::volume volume_left;
        units::mass weight_left;

        int get_var_trade( const item &it, int total_count );
};

namespace npc_trading
{

bool pay_npc( npc &np, int cost );

int cash_to_favor( const npc &, int cash );

void transfer_items( std::vector<item_pricing> &stuff, player &giver, player &receiver,
                     faction *fac, std::list<item_location *> &from_map, bool npc_gives );
double net_price_adjustment( const player &buyer, const player &seller );
bool trade( npc &p, int cost, const std::string &deal );
std::vector<item_pricing> init_selling( npc &p );
std::vector<item_pricing> init_buying( player &buyer, player &seller, bool is_npc );
} // namespace npc_trading

#endif
