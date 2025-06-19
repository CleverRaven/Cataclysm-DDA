#include "kill_tracker.h"

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "cata_variant.h"
#include "character.h"
#include "character_id.h"
#include "event.h"
#include "game.h"
#include "mtype.h"
#include "npc.h"
#include "weakpoint.h"

void kill_tracker::reset( const std::map<mtype_id, int> &kills_,
                          const std::vector<std::string> &npc_kills_ )
{
    kills = kills_;
    npc_kills = npc_kills_;
}

int kill_tracker::kill_count( const mtype_id &mon ) const
{
    auto it = kills.find( mon );
    if( it != kills.end() ) {
        return it->second;
    }
    return 0;
}

int kill_tracker::kill_count( const species_id &spec ) const
{
    int result = 0;
    for( const auto &pair : kills ) {
        if( pair.first->in_species( spec ) ) {
            result += pair.second;
        }
    }
    return result;
}

int kill_tracker::guilt_kill_count( const mtype_id &mon ) const
{
    int count = 0;
    mon_flag_id flag;
    if( mon->has_flag( mon_flag_GUILT_ANIMAL ) ) {
        flag = mon_flag_GUILT_ANIMAL;
    } else if( mon->has_flag( mon_flag_GUILT_CHILD ) ) {
        flag = mon_flag_GUILT_CHILD;
    } else if( mon->has_flag( mon_flag_GUILT_HUMAN ) ) {
        flag = mon_flag_GUILT_HUMAN;
    } else if( mon->has_flag( mon_flag_GUILT_OTHERS ) ) {
        flag = mon_flag_GUILT_OTHERS;
    } else { // worst case scenario when no guilt flags are found
        auto noflag = kills.find( mon );
        if( noflag != kills.end() ) {
            return noflag->second;
        }
    }
    for( const auto &it : kills ) {
        if( it.first->has_flag( flag ) ) {
            count += it.second;
        }
    }
    return count;
}

int kill_tracker::monster_kill_count() const
{
    int result = 0;
    for( const auto &pair : kills ) {
        result += pair.second;
    }
    return result;
}

int kill_tracker::npc_kill_count() const
{
    return npc_kills.size();
}

int kill_tracker::total_kill_count() const
{
    return monster_kill_count() + npc_kill_count();
}

int kill_tracker::legacy_kill_xp() const
{
    int ret = 0;
    for( const std::pair<const mtype_id, int> &pair : kills ) {
        ret += ( pair.first->difficulty + pair.first->difficulty_base ) * pair.second;
    }
    ret += npc_kills.size() * 10;
    return ret;
}

void kill_tracker::clear()
{
    kills.clear();
    npc_kills.clear();
}

static Character *get_avatar_or_follower( const character_id &id )
{
    Character &player = get_player_character();
    if( player.getID() == id ) {
        return &player;
    }
    if( g->get_follower_list().count( id ) ) {
        return g->find_npc( id );
    }
    return nullptr;
}

static constexpr int npc_kill_xp = 10;

void kill_tracker::notify( const cata::event &e )
{
    switch( e.type() ) {
        case event_type::character_kills_monster: {
            const character_id killer_id = e.get<character_id>( "killer" );
            if( Character *killer = get_avatar_or_follower( killer_id ) ) {
                const mtype_id victim_type = e.get<mtype_id>( "victim_type" );
                kills[victim_type]++;
                killer->kill_xp += e.get<int>( "exp" );
                victim_type.obj().families.practice_kill( *killer );
            }
            break;
        }
        case event_type::character_kills_character: {
            const character_id killer_id = e.get<character_id>( "killer" );
            if( Character *killer = get_avatar_or_follower( killer_id ) ) {
                const std::string victim_name = e.get<cata_variant_type::string>( "victim_name" );
                npc_kills.push_back( victim_name );
                killer->kill_xp += npc_kill_xp;
            }
            break;
        }
        default:
            break;
    }
}
