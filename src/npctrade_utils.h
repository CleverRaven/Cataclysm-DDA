#pragma once
#ifndef CATA_SRC_NPCTRADE_UTILS_H
#define CATA_SRC_NPCTRADE_UTILS_H
#include <list>

class npc;
class item;
class time_duration;

constexpr char const *fallback_name = "NPC Shopkeeper unsorted fallback zone";

void add_fallback_zone( npc &guy );
std::list<item> distribute_items_to_npc_zones( std::list<item> &items, npc &guy );
void consume_items_in_zones( npc &guy, time_duration const &elapsed );

#endif // CATA_SRC_NPCTRADE_UTILS_H
