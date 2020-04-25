#pragma once
#ifndef CATA_SRC_CRAFTING_H
#define CATA_SRC_CRAFTING_H

#include <list>

class item;
class player;

enum class craft_flags : int {
    none = 0,
    start_only = 1, // Only require 5% (plus remainder) of tool charges
};

inline constexpr craft_flags operator&( craft_flags l, craft_flags r )
{
    return static_cast<craft_flags>( static_cast<unsigned>( l ) & static_cast<unsigned>( r ) );
}

// removes any (removable) ammo from the item and stores it in the
// players inventory.
void remove_ammo( item &dis_item, player &p );
// same as above but for each item in the list
void remove_ammo( std::list<item> &dis_items, player &p );

#endif // CATA_SRC_CRAFTING_H
