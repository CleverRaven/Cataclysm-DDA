#include "creature_tracker.h"
#include "pathfinding.h"
#include "monster.h"
#include "mongroup.h"
#include "debug.h"
#include "mtype.h"
#include "item.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

Creature_tracker::Creature_tracker()
{
}

Creature_tracker::~Creature_tracker()
{
    clear();
}

monster &Creature_tracker::find( int index )
{
    return const_cast<monster &>( const_cast<const Creature_tracker *>( this )->find( index ) );
}

const monster &Creature_tracker::find( int index ) const
{
    static monster nullmon;
    if( index < 0 || index >= ( int )monsters_list.size() ) {
        debugmsg( "Tried to find monster with invalid index %d. Monster num: %d",
                  index, monsters_list.size() );
        return nullmon;
    }

    return *( monsters_list[index] );
}

int Creature_tracker::mon_at( const tripoint &coords ) const
{
    const auto iter = monsters_by_location.find( coords );
    if( iter != monsters_by_location.end() ) {
        const int critter_id = iter->second;
        if( !monsters_list[critter_id]->is_dead() ) {
            return ( int )critter_id;
        }
    }

    return -1;
}

bool Creature_tracker::add( monster &critter )
{
    if( critter.type->id == NULL_ID ) { // Don't wanna spawn null monsters o.O
        return false;
    }

    if( critter.type->has_flag( MF_VERMIN ) ) {
        // Don't spawn vermin, they aren't implemented yet
        return false;
    }

    const int critter_id = mon_at( critter.pos() );
    if( critter_id != -1 ) {
        // We can spawn stuff on hallucinations, but we need to kill them first
        if( monsters_list[critter_id]->is_hallucination() ) {
            monsters_list[critter_id]->die( nullptr );
            // But don't remove - that would change the monster order and could segfault
        } else if( critter.is_hallucination() ) {
            return false;
        } else {
            debugmsg( "add_zombie: there's already a monster at %d,%d,%d",
                      critter.posx(), critter.posy(), critter.posz() );
            return false;
        }
    }

    if( MonsterGroupManager::monster_is_blacklisted( critter.type->id ) ) {
        return false;
    }

    monsters_by_location[critter.pos()] = monsters_list.size();
    monsters_list.push_back( new monster( critter ) );
    return true;
}

size_t Creature_tracker::size() const
{
    return monsters_list.size();
}

bool Creature_tracker::update_pos( const monster &critter, const tripoint &new_pos )
{
    const auto old_pos = critter.pos();
    if( critter.is_dead() ) {
        // mon_at ignores dead critters anyway, changing their position in the
        // monsters_by_location map is useless.
        remove_from_location_map( critter );
        return true;
    }

    const int critter_id = mon_at( old_pos );
    const int new_critter_id = mon_at( new_pos );
    if( new_critter_id >= 0 ) {
        auto &othermon = *monsters_list[new_critter_id];
        if( othermon.is_hallucination() ) {
            othermon.die( nullptr );
        } else {
            debugmsg( "update_zombie_pos: wanted to move %s to %d,%d,%d, but new location already has %s",
                      critter.disp_name().c_str(),
                      new_pos.x, new_pos.y, new_pos.z, othermon.disp_name().c_str() );
            return false;
        }
    }

    if( critter_id >= 0 ) {
        if( &critter == monsters_list[critter_id] ) {
            monsters_by_location.erase( old_pos );
            monsters_by_location[new_pos] = critter_id;
            return true;
        } else {
            const auto &othermon = *monsters_list[critter_id];
            debugmsg( "update_zombie_pos: wanted to move %s from old location %d,%d,%d, but it had %s instead",
                      critter.disp_name().c_str(),
                      old_pos.x, old_pos.y, old_pos.z, othermon.disp_name().c_str() );
            return false;
        }
    } else {
        // We're changing the x/y/z coordinates of a zombie that hasn't been added
        // to the game yet. add_zombie() will update monsters_by_location for us.
        debugmsg( "update_zombie_pos: no %s at %d,%d,%d (moving to %d,%d,%d)",
                  critter.disp_name().c_str(),
                  old_pos.x, old_pos.y, old_pos.z, new_pos.x, new_pos.y, new_pos.z );
        // Rebuild cache in case the monster actually IS in the game, just bugged
        rebuild_cache();
        return false;
    }

    return false;
}

void Creature_tracker::remove_from_location_map( const monster &critter )
{
    const tripoint &loc = critter.pos();
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
    if( idx < 0 || idx >= ( int )monsters_list.size() ) {
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
        if( elem.second > ( size_t )idx ) {
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
        monsters_by_location[critter.pos()] = i;
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

void Creature_tracker::swap_positions( monster &first, monster &second )
{
    const int first_mdex = mon_at( first.pos() );
    const int second_mdex = mon_at( second.pos() );
    remove_from_location_map( first );
    remove_from_location_map( second );
    bool ok = true;
    if( first_mdex == -1 || second_mdex == -1 || first_mdex == second_mdex ) {
        debugmsg( "Tried to swap monsters with invalid positions" );
        ok = false;
    }

    tripoint temp = second.pos();
    second.spawn( first.pos() );
    first.spawn( temp );
    if( ok ) {
        monsters_by_location[first.pos()] = first_mdex;
        monsters_by_location[second.pos()] = second_mdex;
    } else {
        // Try to avoid spamming error messages if something weird happens
        rebuild_cache();
    }
}

bool Creature_tracker::kill_marked_for_death()
{
    // Important: `Creature::die` must not be called after creature objects (NPCs, monsters) have
    // been removed, the dying creature could still have a pointer (the killer) to another creature.
    bool monster_is_dead = false;
    for( monster *mon : monsters_list ) {
        monster &critter = *mon;
        if( critter.is_dead() ) {
            dbg( D_INFO ) << string_format( "cleanup_dead: critter %d,%d,%d hp:%d %s",
                                            critter.posx(), critter.posy(), critter.posz(),
                                            critter.get_hp(), critter.name().c_str() );
            critter.die( nullptr );
            monster_is_dead = true;
        }
    }

    return monster_is_dead;
}
