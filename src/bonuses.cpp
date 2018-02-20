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

#define dbg(x) DebugLog((DebugLevel)(x),D_MAIN) << __FILE__ << ":" << __LINE__ << ": "

bool needs_damage_type( affected_stat as )
{
    return as == AFFECTED_DAMAGE || as == AFFECTED_ARMOR ||
           as == AFFECTED_ARMOR_PENETRATION;
}

static const std::map<std::string, affected_stat> affected_stat_map = {{
        std::make_pair( "hit", AFFECTED_HIT ),
        std::make_pair( "dodge", AFFECTED_DODGE ),
        std::make_pair( "block", AFFECTED_BLOCK ),
        std::make_pair( "speed", AFFECTED_SPEED ),
        std::make_pair( "movecost", AFFECTED_MOVE_COST ),
        std::make_pair( "damage", AFFECTED_DAMAGE ),
        std::make_pair( "armor", AFFECTED_ARMOR ),
        std::make_pair( "arpen", AFFECTED_ARMOR_PENETRATION ),
        std::make_pair( "target_armor_multiplier", AFFECTED_TARGET_ARMOR_MULTIPLIER )
    }
};

static const std::map<std::string, scaling_stat> scaling_stat_map = {{
        std::make_pair( "str", STAT_STR ),
        std::make_pair( "dex", STAT_DEX ),
        std::make_pair( "int", STAT_INT ),
        std::make_pair( "per", STAT_PER ),
    }
};

scaling_stat scaling_stat_from_string( const std::string &s )
{
    const auto &iter = scaling_stat_map.find( s );
    if( iter == scaling_stat_map.end() ) {
        return STAT_NULL;
    }

    return iter->second;
}

affected_stat affected_stat_from_string( const std::string &s )
{
    const auto &iter = affected_stat_map.find( s );
    if( iter != affected_stat_map.end() ) {
        return iter->second;
    }

    return AFFECTED_NULL;
}

bonus_container::bonus_container()
{
}

void effect_scaling::load( JsonArray &jarr )
{
    if( jarr.test_string() ) {
        stat = scaling_stat_from_string( jarr.next_string() );
    } else {
        stat = STAT_NULL;
    }

    scale = jarr.next_float();
}

void bonus_container::load( JsonObject &jo )
{
    if( jo.has_array( "flat_bonuses" ) ) {
        JsonArray jarr = jo.get_array( "flat_bonuses" );
        load( jarr, false );
    }

    if( jo.has_array( "mult_bonuses" ) ) {
        JsonArray jarr = jo.get_array( "mult_bonuses" );
        load( jarr, true );
    }
}

void bonus_container::load( JsonArray &jarr, bool mult )
{
    while( jarr.has_more() ) {
        JsonArray qualifiers = jarr.next_array();

        affected_stat as;
        damage_type dt = DT_NULL;

        const std::string affected_stat_string = qualifiers.next_string();
        as = affected_stat_from_string( affected_stat_string );
        if( as == AFFECTED_NULL ) {
            jarr.throw_error( "Invalid affected stat" );
        }

        if( needs_damage_type( as ) ) {
            const std::string damage_string = qualifiers.next_string();
            dt = dt_by_name( damage_string );
            if( dt == DT_NULL ) {
                jarr.throw_error( "Invalid damage type" );
            }
        }

        effect_scaling es;
        es.load( qualifiers );
        affected_type at( as, dt );
        // Are we changing multipliers or flats?
        auto &selected = mult ? bonuses_mult : bonuses_flat;
        selected[at].push_back( es );
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

    float ret = 0.0f;
    for( const auto &es : iter->second ) {
        ret += es.get( u );
    }

    return ret;
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

    float ret = 1.0f;
    for( const auto &es : iter->second ) {
        ret *= es.get( u );
    }

    // Currently all relevant effects require non-negative values
    return std::max( 0.0f, ret );
}
float bonus_container::get_mult( const Character &u, affected_stat stat ) const
{
    return get_mult( u, stat, DT_NULL );
}

float effect_scaling::get( const Character &u ) const
{
    float bonus = 0.0f;
    switch( stat ) {
        case STAT_STR:
            bonus = scale * u.get_str();
            break;
        case STAT_DEX:
            bonus = scale * u.get_dex();
            break;
        case STAT_INT:
            bonus = scale * u.get_int();
            break;
        case STAT_PER:
            bonus = scale * u.get_per();
            break;
        case STAT_NULL:
            bonus = scale;
        case NUM_STATS:
            break;
    }

    return bonus;
}
