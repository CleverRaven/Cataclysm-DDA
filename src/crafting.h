#pragma once
#ifndef CATA_SRC_CRAFTING_H
#define CATA_SRC_CRAFTING_H

#include <list>
#include "point.h"

class item;
class player;
class recipe;

enum class craft_flags : int {
    none = 0,
    start_only = 1, // Only require 5% (plus remainder) of tool charges
};

enum class bench_type : int {
    ground = 0,
    hands,
    furniture,
    vehicle
};

inline constexpr craft_flags operator&( craft_flags l, craft_flags r )
{
    return static_cast<craft_flags>( static_cast<unsigned>( l ) & static_cast<unsigned>( r ) );
}

struct bench_location {
    explicit bench_location( bench_type type, tripoint position )
        : type( type ), position( position )
    {}
    bench_type type;
    tripoint position;
};

// removes any (removable) ammo from the item and stores it in the
// players inventory.
void remove_ammo( item &dis_item, player &p );
// same as above but for each item in the list
void remove_ammo( std::list<item> &dis_items, player &p );

bench_location find_best_bench( const player &p, const item &craft );

float workbench_crafting_speed_multiplier( const item &craft, const bench_location &bench );
float crafting_speed_multiplier( const player &p, const recipe &rec, bool in_progress );
float crafting_speed_multiplier( const player &p, const item &craft, const bench_location &bench );
void complete_craft( player &p, item &craft, const bench_location &bench );

#endif // CATA_SRC_CRAFTING_H
