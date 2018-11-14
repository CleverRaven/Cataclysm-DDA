#include "creature_tracker.h"

#include "debug.h"
#include "item.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "string_formatter.h"

#include <algorithm>

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

Creature_tracker::Creature_tracker()
{
}

Creature_tracker::~Creature_tracker() = default;

std::shared_ptr<monster> Creature_tracker::find( const tripoint &pos ) const
{
    const auto iter = monsters_by_location.find( pos );
    if( iter != monsters_by_location.end() ) {
        const std::shared_ptr<monster> &mon_ptr = iter->second;
        if( !mon_ptr->is_dead() ) {
            return mon_ptr;
        }
    }
    return nullptr;
}

bool Creature_tracker::add( monster &critter )
{
    if( critter.type->id.is_null() ) { // Don't want to spawn null monsters o.O
        return false;
    }

    if( critter.type->has_flag( MF_VERMIN ) ) {
        // Don't spawn vermin, they aren't implemented yet
        return false;
    }

    if( check_location( critter.pos() ) == false ) {
        debugmsg( "add_zombie: there's already a monster at %d,%d,%d",
                  critter.posx(), critter.posy(), critter.posz() );
        return false;
    }

    if( MonsterGroupManager::monster_is_blacklisted( critter.type->id ) ) {
        return false;
    }

    monsters_by_location.emplace( critter.pos(), std::make_shared<monster>( critter ) );
    return true;
}

size_t Creature_tracker::size() const
{
    return monsters_by_location.size();
}

bool Creature_tracker::check_location( const tripoint &pos )
{
    const auto mon_iter = monsters_by_location.find( pos );
    if( mon_iter != monsters_by_location.end() ) {
        const std::shared_ptr<monster> &critter_ptr = mon_iter->second;
        if( critter_ptr->is_hallucination() ) {
            critter_ptr->die( nullptr );
        }
        if( critter_ptr->is_dead() ) {
            dead_monsters.push_back( mon_iter->second );
            monsters_by_location.erase( mon_iter );
        } else {
            return false;
        }
    }
    return true;
}

bool Creature_tracker::update_pos( const monster &critter, const tripoint &new_pos )
{
    if( critter.is_dead() ) {
        // find ignores dead critters anyway, changing their position in the
        // monsters_by_location map is useless.
        const auto mon_iter = monsters_by_location.find( critter.pos() );
        dead_monsters.push_back( mon_iter->second );
        monsters_by_location.erase( mon_iter );
        return true;
    }

    if( check_location( new_pos ) == false ) {
        debugmsg( "update_zombie_pos: wanted to move %s to %d,%d,%d, but new location already occupied",
                  critter.disp_name().c_str(), new_pos.x, new_pos.y, new_pos.z );
        return false;
    }

    const auto iter = monsters_by_location.find( critter.pos() );
    if( iter != monsters_by_location.end() ) {
        std::shared_ptr<monster> monster_handle = iter->second;
        monsters_by_location.erase( critter.pos() );
        monsters_by_location[new_pos] = monster_handle;
        return true;
    } else {
        const tripoint &old_pos = critter.pos();
        // We're changing the x/y/z coordinates of a zombie that hasn't been added
        // to the game yet. add_zombie() should try again.
        debugmsg( "update_zombie_pos: no %s at %d,%d,%d (moving to %d,%d,%d)",
                  critter.disp_name().c_str(),
                  old_pos.x, old_pos.y, old_pos.z, new_pos.x, new_pos.y, new_pos.z );
        return false;
    }
}

void Creature_tracker::remove( const monster &critter )
{
    const auto pos_iter = monsters_by_location.find( critter.pos() );
    if( pos_iter != monsters_by_location.end() &&
        pos_iter->second.get() == &critter ) {
        monsters_by_location.erase( pos_iter );
        return;
    }
    const auto iter = std::find_if( monsters_by_location.begin(),
                                    monsters_by_location.end(),
    [&]( const std::pair<tripoint, const std::shared_ptr<monster> &>ptr ) {
        return ptr.second.get() == &critter;
    } );
    if( iter == monsters_by_location.end() ) {
        debugmsg( "Tried to remove invalid monster %s", critter.name().c_str() );
        return;
    }
    monsters_by_location.erase( iter );
}

void Creature_tracker::clear()
{
    monsters_by_location.clear();
    dead_monsters.clear();
}

void Creature_tracker::rebuild_cache()
{
    std::vector < std::shared_ptr<monster> > monster_list;
    monster_list.reserve( size() );
    for( const std::pair<tripoint, std::shared_ptr<monster>> &creature_entry : get_monsters() ) {
        monster_list.push_back( creature_entry.second );
    }
    monsters_by_location.clear();
    for( const std::shared_ptr<monster> &mon_ptr : monster_list ) {
        monsters_by_location[mon_ptr->pos()] = mon_ptr;
    }
}

void Creature_tracker::swap_positions( monster &first, monster &second )
{
    if( first.pos() == second.pos() ) {
        return;
    }

    // Either of them may be invalid!
    const auto first_iter = monsters_by_location.find( first.pos() );
    const auto second_iter = monsters_by_location.find( second.pos() );
    // implied: first_iter != second_iter

    std::shared_ptr<monster> first_ptr;
    if( first_iter != monsters_by_location.end() ) {
        first_ptr = first_iter->second;
        monsters_by_location.erase( first_iter );
    }

    std::shared_ptr<monster> second_ptr;
    if( second_iter != monsters_by_location.end() ) {
        second_ptr = second_iter->second;
        monsters_by_location.erase( second_iter );
    }
    // implied: (first_ptr != second_ptr) or (first_ptr == nullptr && second_ptr == nullptr)

    tripoint temp = second.pos();
    second.spawn( first.pos() );
    first.spawn( temp );

    // If the pointers have been taken out of the list, put them back in.
    if( first_ptr ) {
        monsters_by_location[first.pos()] = first_ptr;
    }
    if( second_ptr ) {
        monsters_by_location[second.pos()] = second_ptr;
    }
}

static void kill_monster( monster &critter, bool &monster_is_dead )
{
    dbg( D_INFO ) << string_format( "cleanup_dead: critter %d,%d,%d hp:%d %s",
                                    critter.posx(), critter.posy(), critter.posz(),
                                    critter.get_hp(), critter.name().c_str() );
    critter.die( nullptr );
    monster_is_dead = true;
}

bool Creature_tracker::kill_marked_for_death()
{
    // Important: `Creature::die` must not be called after creature objects (NPCs, monsters) have
    // been removed, the dying creature could still have a pointer (the killer) to another creature.
    bool monster_is_dead = false;
    for( const std::pair<tripoint, std::shared_ptr<monster>> &monster_entry :
         monsters_by_location ) {
        monster &critter = *monster_entry.second;
        if( critter.is_dead() ) {
            kill_monster( critter, monster_is_dead );
        }
    }
    for( const std::shared_ptr<monster> &dead_monster : dead_monsters ) {
        monster &critter = *dead_monster;
        kill_monster( critter, monster_is_dead );
    }

    return monster_is_dead;
}

void Creature_tracker::remove_dead()
{
    // Can't use game::all_monsters() as it would not contain *dead* monsters.
    for( auto iter = monsters_by_location.begin(); iter != monsters_by_location.end(); ) {
        const monster &critter = *( iter->second );
        if( critter.is_dead() ) {
            iter = monsters_by_location.erase( iter );
        } else {
            ++iter;
        }
    }
    dead_monsters.clear();
}
