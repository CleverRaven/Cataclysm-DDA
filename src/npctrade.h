#pragma once
#ifndef CATA_SRC_NPCTRADE_H
#define CATA_SRC_NPCTRADE_H

#include <cstddef>
#include <iosfwd>
#include <list>
#include <type_traits>
#include <vector>

#include "cursesdef.h"
#include "inventory.h"
#include "item_location.h"
#include "units.h"

class Character;
class faction;
class item;
class npc;
class player;
class ui_adaptor;

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
        void adjust_values( double adjust, const faction *fac );
        // recursively fetches all contents of the item depth first
        std::vector<item_pricing *> get_contents_rec();
        static void populate_container_pointers( std::vector<item_pricing> &trading );

        item_location loc;
        int price = 0;
        // Whether this is selected for trading. Item's price, volume and weight impact
        // will be calculated. Item will be transferred after trading is done.
        bool selected = false;
        // Items inside selected containers are marked but not selected.
        // This way they are highlighted in the UI, but their price is not calculated
        // and don't get duplicately transferred after the trade has been done.
        // Core assumption is, that the container price includes the combined prices of all items inside.
        bool marked = false;
        bool is_container = false;
        // top-level contents, i.e doesn't return contents of contents, but you can manually get these.
        std::vector<item_pricing *> contents;
        item_pricing *parent;
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
        int your_balance = 0;

        void setup_trade( int cost, npc &np );
        bool perform_trade( npc &np, const std::string &deal );
        void update_npc_owed( npc &np );

    private:
        void setup_win( ui_adaptor &ui );
        void update_win( npc &np, const std::string &deal );
        void show_item_data( size_t offset, std::vector<item_pricing> &target_list );
        void adjust_balance( item_pricing &ip, npc &np, int change_amount );
        int get_change_amount( item_pricing &ip, bool manual );

        catacurses::window w_head;
        catacurses::window w_them;
        catacurses::window w_you;
        size_t entries_per_page = 0;
        bool focus_them = true; // Is the focus on them?
        size_t them_off = 0, you_off = 0; // Offset from the start of the list

        inventory temp;
        units::volume volume_left;
        units::mass weight_left;

        int get_var_trade( const item &it, int total_count );
        bool npc_will_accept_trade( const npc &np ) const;
        int calc_npc_owes_you( const npc &np ) const;
};

namespace npc_trading
{

bool pay_npc( npc &np, int cost );

int cash_to_favor( const npc &, int cash );

std::list<item> transfer_items( std::vector<item_pricing> &stuff, player &giver, player &receiver,
                                std::list<item_location *> &from_map, bool npc_gives );
double net_price_adjustment( const player &buyer, const player &seller );
bool trade( npc &p, int cost, const std::string &deal );
std::vector<item_pricing> init_selling( npc &p );
std::vector<item_pricing> init_buying( player &buyer, player &seller, bool is_npc );
} // namespace npc_trading

#endif // CATA_SRC_NPCTRADE_H
