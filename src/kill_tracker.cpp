#include "kill_tracker.h"

#include <memory>
#include <tuple>
#include <utility>

#include "avatar.h"
#include "cata_variant.h"
#include "character_id.h"
#include "color.h"
#include "event.h"
#include "game.h"
#include "mtype.h"
#include "options.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"

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

int kill_tracker::kill_xp() const
{
    int ret = 0;
    for( const std::pair<const mtype_id, int> &pair : kills ) {
        ret += ( pair.first->difficulty + pair.first->difficulty_base ) * pair.second;
    }
    ret += npc_kills.size() * 10;
    return ret;
}

std::string kill_tracker::get_kills_text() const
{
    std::vector<std::string> data;
    int totalkills = 0;

    std::map<std::tuple<std::string, std::string, nc_color>, int> kill_counts;

    // map <name, sym, color> to kill count
    for( const std::pair<const mtype_id, int> &elem : kills ) {
        const mtype &m = elem.first.obj();
        auto key = std::make_tuple( m.nname(), m.sym, m.color );
        kill_counts[key] += elem.second;
        totalkills += elem.second;
    }

    for( const auto &entry : kill_counts ) {
        const int num_kills = entry.second;
        const std::string &mname = std::get<0>( entry.first );
        const std::string &symbol = std::get<1>( entry.first );
        const nc_color color = std::get<2>( entry.first );
        data.push_back( string_format( "%4d ", num_kills ) + colorize( symbol,
                        color ) + " " + colorize( mname, c_light_gray ) );
    }
    for( const auto &npc_name : npc_kills ) {
        totalkills += 1;
        data.push_back( string_format( "%4d ", 1 ) + colorize( "@ " + npc_name, c_magenta ) );
    }
    std::string buffer;
    if( data.empty() ) {
        buffer = _( "You haven't killed any monsters yet!" );
    } else {
        buffer = string_format( _( "KILL COUNT: %d" ), totalkills );
        if( get_option<bool>( "STATS_THROUGH_KILLS" ) ) {
            buffer += string_format( _( "\nExperience: %d (%d points available)" ), kill_xp(),
                                     g->u.free_upgrade_points() );
        }
        buffer += "\n";
    }
    for( const std::string &line : data ) {
        buffer += "\n" + line;
    }
    return buffer;
}

void kill_tracker::clear()
{
    kills.clear();
    npc_kills.clear();
}

void kill_tracker::notify( const cata::event &e )
{
    switch( e.type() ) {
        case event_type::character_kills_monster: {
            character_id killer = e.get<character_id>( "killer" );
            if( killer != g->u.getID() ) {
                // TODO: add a kill counter for npcs?
                break;
            }
            mtype_id victim_type = e.get<mtype_id>( "victim_type" );
            kills[victim_type]++;
            break;
        }
        case event_type::character_kills_character: {
            character_id killer = e.get<character_id>( "killer" );
            if( killer != g->u.getID() ) {
                break;
            }
            std::string victim_name = e.get<cata_variant_type::string>( "victim_name" );
            npc_kills.push_back( victim_name );
            break;
        }
        default:
            break;
    }
}
