#include "creature_tracker.h"
#include "monster.h"
#include "mongroup.h"
#include "debug.h"

Creature_tracker::Creature_tracker()
{
}

Creature_tracker::~Creature_tracker()
{
    clear();
}

monster &Creature_tracker::find(int index)
{
    static monster nullmon;
    if( index < 0 || index >= (int)monsters_list.size() ) {
        debugmsg( "Tried to find monster with invalid index %d. Monster num: %d",
                  index, monsters_list.size() );
        return nullmon;
    }

    return *(monsters_list[index]);
}

int Creature_tracker::mon_at( const tripoint &coords ) const
{
    const auto iter = monsters_by_location.find( coords );
    if( iter != monsters_by_location.end() ) {
        const int critter_id = iter->second;
        if( !monsters_list[critter_id]->is_dead() ) {
            return (int)critter_id;
        }
    }

    return -1;
}

bool Creature_tracker::add( monster &critter )
{
    if( critter.type->id == "mon_null" ) { // Don't wanna spawn null monsters o.O
        return false;
    }

    if( -1 != mon_at( critter.pos3() ) ) {
        debugmsg( "add_zombie: there's already a monster at %d,%d,%d", 
                  critter.posx(), critter.posy(), critter.posz() );
        return false;
    }

    if( MonsterGroupManager::monster_is_blacklisted(critter.type) ) {
        return false;
    }

    monsters_by_location[critter.pos3()] = monsters_list.size();
    monsters_list.push_back(new monster(critter));
    return true;
}

size_t Creature_tracker::size() const
{
    return monsters_list.size();
}

bool Creature_tracker::update_pos(const monster &critter, const tripoint &new_pos)
{
    const auto old_pos = critter.pos();
    if( critter.is_dead() ) {
        // mon_at ignores dead critters anyway, changing their position in the
        // monsters_by_location map is useless.
        remove_from_location_map( critter );
        return true;
    }

    bool success = false;
    const int critter_id = mon_at( old_pos );
    const int new_critter_id = mon_at( new_pos );
    if( new_critter_id >= 0 ) {
        const auto &othermon = *monsters_list[new_critter_id];
        debugmsg( "update_zombie_pos: wanted to move %s to %d,%d,%d, but new location already has %s",
                  critter.disp_name().c_str(),
                  new_pos.x, new_pos.y, new_pos.z, othermon.disp_name().c_str() );
    } else if( critter_id >= 0 ) {
        if( &critter == monsters_list[critter_id] ) {
            monsters_by_location.erase( old_pos );
            monsters_by_location[new_pos] = critter_id;
            success = true;
        } else {
            const auto &othermon = *monsters_list[critter_id];
            debugmsg( "update_zombie_pos: wanted to move %s from old location %d,%d,%d, but it had %s instead",
                      critter.disp_name().c_str(),
                      old_pos.x, old_pos.y, old_pos.z, othermon.disp_name().c_str() );
        }
    } else {
        // We're changing the x/y/z coordinates of a zombie that hasn't been added
        // to the game yet. add_zombie() will update monsters_by_location for us.
        debugmsg("update_zombie_pos: no %s at %d,%d,%d (moving to %d,%d,%d)",
                 critter.disp_name().c_str(),
                 old_pos.x, old_pos.y, old_pos.z, new_pos.x, new_pos.y, new_pos.z );
        // Rebuild cache in case the monster actually IS in the game, just bugged
        rebuild_cache();
    }

    return success;
}

void Creature_tracker::remove_from_location_map( const monster &critter )
{
    const tripoint &loc = critter.pos3();
    const auto pos_iter = monsters_by_location.find( loc );
    if( pos_iter != monsters_by_location.end() ) {
        const auto &other = find( pos_iter->second );
        if( &other == &critter ) {
            monsters_by_location.erase( pos_iter );
        }
    }
}

void Creature_tracker::remove( const int idx )
{
    if( idx < 0 || idx >= (int)monsters_list.size() ) {
        debugmsg( "Tried to remove monster with invalid index %d. Monster num: %d",
                  idx, monsters_list.size() );
        return;
    }

    monster &m = *monsters_list[idx];
    remove_from_location_map( m );

    delete monsters_list[idx];
    monsters_list.erase( monsters_list.begin() + idx );

    // Fix indices in monsters_by_location for any zombies that were just moved down 1 place.
    for( auto &elem : monsters_by_location ) {
        if( elem.second > (size_t)idx ) {
            --elem.second;
        }
    }
}

void Creature_tracker::clear()
{
    for( auto monster_ptr : monsters_list ) {
        delete monster_ptr;
    }
    monsters_list.clear();
    monsters_by_location.clear();
}

void Creature_tracker::rebuild_cache()
{
    monsters_by_location.clear();
    for( size_t i = 0; i < monsters_list.size(); i++ ) {
        monster &critter = *monsters_list[i];
        monsters_by_location[critter.pos3()] = i;
    }
}

const std::vector<monster> &Creature_tracker::list() const
{
    static std::vector<monster> for_now;
    for_now.clear();
    for( const auto monster_ptr : monsters_list ) {
        for_now.push_back( *monster_ptr );
    }
    return for_now;
}
