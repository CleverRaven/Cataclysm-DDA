#pragma once
#ifndef NPCTRADE_H
#define NPCTRADE_H

#include <vector>
#include <set>
#include <string>
#include <utility>

#include "inventory.h"
#include "item_location.h"

class Character;
class item;
class npc;
class player;

struct item_pricing {
    item_pricing( Character &c, item *it, int v, bool s ) : loc( c, it ), price( v ), selected( s ) {
    }

    item_pricing( item_location &&l, int v, bool s ) : loc( std::move( l ) ), price( v ),
        selected( s ) {
    }

    item_location loc;
    int price;
    // Whether this is selected for trading, init_buying and init_selling initialize
    // this to `false`.
    bool selected;
};

int cash_to_favor( const npc &, int cash );

inventory inventory_exchange( inventory &inv,
                              const std::set<item *> &without, const std::vector<item *> &added );
std::vector<item_pricing> init_selling( npc &p );
std::vector<item_pricing> init_buying( npc &p, player &u );
bool trade( npc &p, int cost, const std::string &deal );

#endif
