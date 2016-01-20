#include "item.h"
#include "monster.h"
#include "game.h"
#include "map.h"
#include "damage.h"
#include "rng.h"
#include "debug.h"
#include "map_iterator.h"
#include "field.h"
#include "mtype.h"

#include <map>

damage_instance::damage_instance() { }
damage_instance damage_instance::physical( float bash, float cut, float stab, int arpen )
{
    damage_instance d;
    d.add_damage( DT_BASH, bash, arpen );
    d.add_damage( DT_CUT, cut, arpen );
    d.add_damage( DT_STAB, stab, arpen );
    return d;
}
damage_instance::damage_instance( damage_type dt, float a, int rp, float rm, float mul )
{
    add_damage( dt, a, rp, rm, mul );
}

void damage_instance::add_damage( damage_type dt, float a, int rp, float rm, float mul )
{
    damage_unit du( dt, a, rp, rm, mul );
    damage_units.push_back( du );
}
void damage_instance::add_effect( std::string effect )
{
    effects.insert( effect );
}

void damage_instance::mult_damage( double multiplier )
{
    for( auto &elem : damage_units ) {
        elem.damage_multiplier *= multiplier;
    }
}
float damage_instance::type_damage( damage_type dt ) const
{
    float ret = 0;
    for( const auto &elem : damage_units ) {
        if( elem.type == dt ) {
            ret += elem.amount * elem.damage_multiplier;
        }
    }
    return ret;
}
//This returns the damage from this damage_instance. The damage done to the target will be reduced by their armor.
float damage_instance::total_damage() const
{
    float ret = 0;
    for( const auto &elem : damage_units ) {
        ret += elem.amount * elem.damage_multiplier;
    }
    return ret;
}
void damage_instance::clear()
{
    damage_units.clear();
    effects.clear();
}

dealt_damage_instance::dealt_damage_instance() : dealt_dams( NUM_DT, 0 )
{
    dealt_dams.resize( NUM_DT );
}

dealt_damage_instance::dealt_damage_instance( std::vector<int> &dealt ) : dealt_dams( dealt )
{
    dealt_dams.resize( NUM_DT );
}
void dealt_damage_instance::set_damage( damage_type dt, int amount )
{
    if( dt < 0 || dt >= NUM_DT ) {
        debugmsg( "Tried to set invalid damage type %d. NUM_DT is %d", dt, NUM_DT );
        return;
    }

    dealt_dams[dt] = amount;
}
int dealt_damage_instance::type_damage( damage_type dt ) const
{
    if( ( size_t )dt < dealt_dams.size() ) {
        return dealt_dams[dt];
    }

    return 0;
}
int dealt_damage_instance::total_damage() const
{
    return std::accumulate( dealt_dams.begin(), dealt_dams.end(), 0 );
}


resistances::resistances() : resist_vals( NUM_DT, 0 ) { }
resistances::resistances( item &armor, bool to_self ) : resist_vals( NUM_DT, 0 )
{
    // Armors protect, but all items can resist
    if( to_self || armor.is_armor() ) {
        for( int i = 0; i < NUM_DT; i++ ) {
            damage_type dt = static_cast<damage_type>( i );
            set_resist( dt, armor.damage_resist( dt, to_self ) );
        }
    }
}
resistances::resistances( monster &monster ) : resist_vals( NUM_DT, 0 )
{
    set_resist( DT_BASH, monster.type->armor_bash );
    set_resist( DT_CUT,  monster.type->armor_cut );
    set_resist( DT_STAB, monster.type->armor_stab );
    set_resist( DT_ACID, monster.type->armor_acid );
    set_resist( DT_HEAT, monster.type->armor_fire );
}
void resistances::set_resist( damage_type dt, int amount )
{
    resist_vals[dt] = amount;
}
int resistances::type_resist( damage_type dt ) const
{
    return resist_vals[dt];
}
float resistances::get_effective_resist( const damage_unit &du ) const
{
    float effective_resist;
    switch( du.type ) {
        case DT_BASH:
            effective_resist = std::max( type_resist( DT_BASH ) - du.res_pen, 0 ) * du.res_mult;
            break;
        case DT_CUT:
            effective_resist = std::max( type_resist( DT_CUT ) - du.res_pen, 0 ) * du.res_mult;
            break;
        case DT_STAB:
            effective_resist = std::max( type_resist( DT_STAB ) - du.res_pen, 0 ) * du.res_mult;
            break;
        case DT_ACID:
            effective_resist = std::max( type_resist( DT_ACID ) - du.res_pen, 0 ) * du.res_mult;
            break;
        case DT_HEAT:
            effective_resist = std::max( type_resist( DT_HEAT ) - du.res_pen, 0 ) * du.res_mult;
            break;
        default: // TODO: Other types
            effective_resist = 0;
    }
    return effective_resist;
}

void ammo_effects( const tripoint &p, const std::set<std::string> &effects )
{
    if( effects.count( "EXPLOSIVE_SMALL" ) > 0 ) {
        g->explosion( p, 24, 0.4 );
    }

    if( effects.count( "EXPLOSIVE" ) > 0 ) {
        g->explosion( p, 24 );
    }

    if( effects.count( "FRAG" ) > 0 ) {
        g->explosion( p, 24, 0.4, 28, false );
    }

    if( effects.count( "NAPALM" ) > 0 ) {
        g->explosion( p, 4, 0.7, 0, true );
        // More intense fire near the center
        for( auto && pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_fire, 1, 0 );
        }
    }

    if( effects.count( "NAPALM_BIG" ) > 0 ) {
        g->explosion( p, 24, 0.8, 0, true );
        // More intense fire near the center
        for( auto && pt : g->m.points_in_radius( p, 3, 0 ) ) {
            g->m.add_field( pt, fd_fire, 1, 0 );
        }
    }

    if( effects.count( "MININUKE_MOD" ) > 0 ) {
        g->explosion( p, 450 );
        for( auto && pt : g->m.points_in_radius( p, 6, 0 ) ) {
            if( g->m.sees( p, pt, 3 ) &&
                g->m.passable( pt ) ) {
                g->m.add_field( pt, fd_nuke_gas, 3, 0 );
            }
        }
    }

    if( effects.count( "ACIDBOMB" ) > 0 ) {
        for( auto && pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_acid, 3, 0 );
        }
    }

    if( effects.count( "EXPLOSIVE_BIG" ) > 0 ) {
        g->explosion( p, 40 );
    }

    if( effects.count( "EXPLOSIVE_HUGE" ) > 0 ) {
        g->explosion( p, 80 );
    }

    if( effects.count( "TOXICGAS" ) > 0 ) {
        for( auto && pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_toxic_gas, 3, 0 );
        }
    }
    if( effects.count( "TEARGAS" ) > 0 ) {
        for( auto && pt : g->m.points_in_radius( p, 2, 0 ) ) {
            g->m.add_field( pt, fd_tear_gas, 3, 0 );
        }
    }
    if( effects.count( "GAS_FUNGICIDAL" ) > 0 ) {
        for( auto && pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_fungicidal_gas, 3, 0 );
        }
    }
    if( effects.count( "SMOKE" ) > 0 ) {
        for( auto && pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_smoke, 3, 0 );
        }
    }
    if( effects.count( "SMOKE_BIG" ) > 0 ) {
        for( auto && pt : g->m.points_in_radius( p, 6, 0 ) ) {
            g->m.add_field( pt, fd_smoke, 18, 0 );
        }
    }

    if( effects.count( "FLASHBANG" ) ) {
        g->flashbang( p );
    }

    if( effects.count( "NO_BOOM" ) == 0 && effects.count( "FLAME" ) > 0 ) {
        for( auto && pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_fire, 1, 0 );
        }
    }

    if( effects.count( "FLARE" ) > 0 ) {
        g->m.add_field( p, fd_fire, 1, 0 );
    }

    if( effects.count( "LIGHTNING" ) > 0 ) {
        for( auto && pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_electricity, 3, 0 );
        }
    }

    if( effects.count( "PLASMA" ) > 0 ) {
        for( auto && pt : g->m.points_in_radius( p, 1, 0 ) ) {
            if( one_in( 2 ) ) {
                g->m.add_field( pt, fd_plasma, rng( 2, 3 ), 0 );
            }
        }
    }

}


int aoe_size( const std::set<std::string> &tags )
{
    if( tags.count( "NAPALM_BIG" ) ||
        tags.count( "EXPLOSIVE_HUGE" ) ) {
        return 4;
    } else if( tags.count( "NAPALM" ) ||
               tags.count( "EXPLOSIVE_BIG" ) ) {
        return 3;
    } else if( tags.count( "EXPLOSIVE" ) ||
               tags.count( "EXPLOSIVE_SMALL" ) ||
               tags.count( "FRAG" ) ) {
        return 2;
    } else if( tags.count( "ACIDBOMB" ) ||
               tags.count( "FLAME" ) ) {
        return 1;
    }


    return 0;
}


static const std::map<std::string, damage_type> dt_map = {
    { "true", DT_TRUE },
    { "biological", DT_BIOLOGICAL },
    { "bash", DT_BASH },
    { "cut", DT_CUT },
    { "acid", DT_ACID },
    { "stab", DT_STAB },
    { "heat", DT_HEAT },
    { "cold", DT_COLD },
    { "electric", DT_ELECTRIC }
};

damage_type dt_by_name( const std::string &name )
{
    const auto &iter = dt_map.find( name );
    if( iter == dt_map.end() ) {
        return DT_NULL;
    }

    return iter->second;
}

projectile::projectile() :
    speed( 0 ),
    drop( nullptr )
{ }

projectile::projectile( const projectile &other )
{
    ( *this ) = other;
}

projectile &projectile::operator=( const projectile &other )
{
    impact = other.impact;
    speed = other.speed;
    proj_effects = other.proj_effects;
    set_drop( other.get_drop() );

    return *this;
}

const item &projectile::get_drop() const
{
    if( drop != nullptr ) {
        return *drop;
    }

    static const item null_drop;
    return null_drop;
}

void projectile::set_drop( const item &it )
{
    if( it.is_null() ) {
        unset_drop();
    } else {
        drop.reset( new item( it ) );
    }
}

void projectile::set_drop( item &&it )
{
    if( it.is_null() ) {
        unset_drop();
    } else {
        drop.reset( new item( std::move( it ) ) );
    }
}

void projectile::unset_drop()
{
    drop.reset( nullptr );
}

