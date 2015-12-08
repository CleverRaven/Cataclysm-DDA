#include "monster_group.h"

#include <algorithm>

static bool cmp_monster_by_update( const monster &lhs, const monster &rhs ) {
    return lhs.next_overmap_move() < rhs.next_overmap_move();
}

void monster_group::add_monster( monster &new_monster ) {
    // Reset the strongest stimulus since this new monster might not have received it.
    // TODO: make this fast?
    strongest_stimulus = null_monster_stimulus;
    monster_list.push_back( new_monster );
    std::push_heap( monster_list.begin(), monster_list.end(), cmp_monster_by_update );
}

void monster_group::add_monster_group( monster_group &new_monsters ) {
    // Reset the strongest stimulus since this new monster might not have received it.
    strongest_stimulus = null_monster_stimulus;
    // Might be faster to append them all, then make_heap.
    for( monster new_monster : new_monsters.monster_list ) {
        monster_list.push_back( new_monster );
        std::push_heap( monster_list.begin(), monster_list.end(), cmp_monster_by_update );
    }
}

// We track the strongest stimulus encountered at this point since the last monster acted.
// We only need to apply the strongest stimulus since the last move to the whole monster list.
void monster_group::apply_stimulus( const tripoint &location, const monster_group_stimulus &stimulus ) {
    // Very important, this does not affect the ordering of monsters within monster_list.
    // If it did, we would have to re-heap afterwards.
    // TODO: Scale for distance or similar?
    if( strongest_stimulus.strength < stimulus.strength ) {
        strongest_stimulus = stimulus;
        for( auto &member : monster_list ) {
            member.apply_stimulus( location, stimulus );
        }
    }
}

std::map<tripoint, monster_group> monster_group::move(
    const tripoint &location, const int turn_number, const senses &sensor ) {
    std::map<tripoint, monster_group> moving_monsters;
    while( !monster_list.empty() && monster_list.begin()->next_overmap_move() <= turn_number ) {
        monster mon = *monster_list.begin();
        monster_list.erase( monster_list.begin() );
        tripoint destination = mon.overmap_move( location, sensor );
        moving_monsters[destination].add_monster( mon );
    }
    return moving_monsters;
}

void monster_group::serialize( JsonOut & ) const {
}

void monster_group::deserialize( JsonIn & ) {
}
