#include "bonuses.h"
#include "damage.h"
#include "json.h"
#include "character.h"
#include "debug.h"
#include <map>
#include <string>
#include <utility>
#include <sstream>
#include <vector>

// Tokenize
void split( const std::string &s, char delim, std::vector<std::string> &elems )
{
    std::stringstream ss( s );
    std::string item;
    while( std::getline( ss, item, delim ) ) {
        elems.push_back( item );
    }
}

std::vector<std::string> split( const std::string &s, char delim )
{
    std::vector<std::string> elems;
    split( s, delim, elems );
    return elems;
}

bool needs_damage_type( affected_stat as )
{
    return as == AFFECTED_DAMAGE || as == AFFECTED_ARMOR ||
           as == AFFECTED_ARMOR_PENETRATION;
}

scaling_stat scaling_stat_from_string( const std::string &s )
{
    static const std::map<std::string, scaling_stat> stat_map = {{
            std::make_pair( "str", STAT_STR ),
            std::make_pair( "dex", STAT_DEX ),
            std::make_pair( "int", STAT_INT ),
            std::make_pair( "per", STAT_PER ),
        }
    };

    const auto &iter = stat_map.find( s );
    if( iter == stat_map.end() ) {
        return STAT_NULL;
    }

    return iter->second;
}

affected_stat affected_stat_from_string( const std::string &s )
{
    static const std::map<std::string, affected_stat> stat_map = {{
            std::make_pair( "hit", AFFECTED_HIT ),
            std::make_pair( "dodge", AFFECTED_DODGE ),
            std::make_pair( "block", AFFECTED_BLOCK ),
            std::make_pair( "speed", AFFECTED_SPEED ),
            std::make_pair( "movecost", AFFECTED_MOVE_COST ),
            std::make_pair( "damage", AFFECTED_DAMAGE ),
            std::make_pair( "arm", AFFECTED_ARMOR ),
            std::make_pair( "arpen", AFFECTED_ARMOR_PENETRATION )
        }
    };
    const auto &iter = stat_map.find( s );
    if( iter != stat_map.end() ) {
        return iter->second;
    }

    return AFFECTED_NULL;
}

bonus_container::bonus_container()
{
}

// Duplicates cut bonuses into stab bonuses
void bonus_container::legacy_fixup( bonus_map &bm )
{
    bool changed_something = false;
    do {
        for( const auto &pr : bm ) {
            if( pr.first.get_damage_type() != DT_CUT ) {
                continue;
            }

            const affected_type as( pr.first.get_stat(), DT_STAB );
            if( bm.count( as ) != 0 ) {
                continue;
            }

            changed_something = true;
            const auto value_copy = pr.second;
            bm[as] = value_copy;
            // Iterators are now scrambled, we need to restart the loop
            break;
        }
    } while( changed_something );
}

void bonus_container::load( JsonObject &jo )
{
    // Warning: ugly string hackery
    static const std::string mult( "mult" );
    const auto &members = jo.get_member_names();
    for( const auto &member : members ) {
        const std::vector<std::string> tokens = split( member, '_' );
        if( tokens.size() < 2 ) {
            continue;
        }

        affected_stat as = AFFECTED_NULL;
        scaling_stat ss = STAT_NULL;
        damage_type dt = DT_NULL;
        bool is_mult = false;
        bool skipped_token = false;
        for( const auto &token : tokens ) {
            if( token == mult ) {
                is_mult = true;
                continue;
            }

            damage_type ndt = dt_by_name( token );
            if( ndt != DT_NULL ) {
                if( dt != DT_NULL ) {
                    jo.throw_error( "Damage type set twice", member );
                }
                dt = ndt;
                continue;
            }

            affected_stat nas = affected_stat_from_string( token );
            if( nas != AFFECTED_NULL ) {
                if( as != AFFECTED_NULL ) {
                    jo.throw_error( "Affected stat set twice", member );
                }
                as = nas;
                continue;
            }

            scaling_stat nss = scaling_stat_from_string( token );
            if( nss != STAT_NULL ) {
                if( ss != STAT_NULL ) {
                    jo.throw_error( "Scaling stat set twice", member );
                }
                ss = nss;
                continue;
            }

            skipped_token = true;
            break;
        }

        if( skipped_token ) {
            // This isn't a set of ma_ modifiers
            continue;
        }

        if( dt != DT_NULL && as == AFFECTED_NULL ) {
            // By default, we want to affect damage
            // For example, "bash_mult", "bash_str", etc.
            as = AFFECTED_DAMAGE;
        }

        if( needs_damage_type( as ) ) {
            if( dt == DT_NULL ) {
                jo.throw_error( "Damage type required but unset", member );
            }
        } else if( dt != DT_NULL ) {
            jo.throw_error( "Damage type not required but set", member );
        }

        const float val = jo.get_float( member );

        affected_type at( as, dt );
        // Are we changing mults or flats?
        auto &selected = is_mult ? bonuses_mult : bonuses_flat;
        const bool had = selected.count( at );
        effect_scaling &es = selected[at];
        if( is_mult && !had ) {
            // If unset, multipliers need to start at 0
            es.base = 1.0f;
        }

        if( ss != STAT_NULL ) {
            es.stat = ss;
            es.scale = val;
        } else {
            es.base = val;
        }
    }
}

affected_type::affected_type( affected_stat s )
{
    stat = s;
}

affected_type::affected_type( affected_stat s, damage_type t )
{
    stat = s;
    if( needs_damage_type( s ) ) {
        type = t;
    } else {
        type = DT_NULL;
    }
}

bool affected_type::operator<( const affected_type &other ) const
{
    return stat < other.stat || ( stat == other.stat && type < other.type );
}
bool affected_type::operator==( const affected_type &other ) const
{
    // NOTE: Constructor has to ensure type is set to NULL for some stats
    return stat == other.stat && type == other.type;
}

float bonus_container::get_flat( const Character &u, affected_stat stat, damage_type dt ) const
{
    affected_type type( stat, dt );
    const auto &iter = bonuses_flat.find( type );
    if( iter == bonuses_flat.end() ) {
        return 0.0f;
    }

    return iter->second.get( u );
}
float bonus_container::get_flat( const Character &u, affected_stat stat ) const
{
    return get_flat( u, stat, DT_NULL );
}

float bonus_container::get_mult( const Character &u, affected_stat stat, damage_type dt ) const
{
    affected_type type( stat, dt );
    const auto &iter = bonuses_mult.find( type );
    if( iter == bonuses_mult.end() ) {
        return 1.0f;
    }

    return iter->second.get( u );
}
float bonus_container::get_mult( const Character &u, affected_stat stat ) const
{
    return get_mult( u, stat, DT_NULL );
}

float effect_scaling::get( const Character &u ) const
{
    float bonus = base;
    switch( stat ) {
        case STAT_STR:
            bonus += scale * u.get_str();
            break;
        case STAT_DEX:
            bonus += scale * u.get_dex();
            break;
        case STAT_INT:
            bonus += scale * u.get_int();
            break;
        case STAT_PER:
            bonus += scale * u.get_per();
            break;
        case STAT_NULL:
        case NUM_STATS:
            break;
    }

    return bonus;
}
