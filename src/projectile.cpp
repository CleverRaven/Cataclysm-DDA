#include "projectile.h"

#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "explosion.h"
#include "field.h"
#include "rng.h"
#include "item.h"

projectile::projectile() :
    speed( 0 ), range( 0 ), momentum_loss( 0.125f ), drop( nullptr ), custom_explosion( nullptr )
{ }

projectile::~projectile() = default;

projectile::projectile( const projectile &other )
{
    ( *this ) = other;
}

projectile &projectile::operator=( const projectile &other )
{
    impact = other.impact;
    speed = other.speed;
    range = other.range;
    momentum_loss = other.momentum_loss;
    proj_effects = other.proj_effects;
    set_drop( other.get_drop() );
    set_custom_explosion( other.get_custom_explosion() );

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

const explosion_data &projectile::get_custom_explosion() const
{
    if( custom_explosion != nullptr ) {
        return *custom_explosion;
    }

    static const explosion_data null_explosion{};
    return null_explosion;
}

void projectile::set_custom_explosion( const explosion_data &ex )
{
    custom_explosion.reset( new explosion_data( ex ) );
}

void projectile::unset_custom_explosion()
{
    custom_explosion.reset();
}

void apply_ammo_effects( const tripoint &p, const std::set<std::string> &effects )
{
    if( effects.count( "EXPLOSIVE_SMALL" ) > 0 ) {
        g->explosion( p, 24, 0.4 );
    }

    if( effects.count( "EXPLOSIVE" ) > 0 ) {
        g->explosion( p, 24 );
    }

    if( effects.count( "FRAG" ) > 0 ) {
        explosion_data frag;
        frag.power = 1.0f;
        frag.shrapnel.count = 50;
        frag.shrapnel.mass = 5;
        frag.shrapnel.recovery = 100;
        frag.shrapnel.drop = "shrapnel";
        g->explosion( p, frag );
    }

    if( effects.count( "NAPALM" ) > 0 ) {
        g->explosion( p, 4, 0.7, true );
        // More intense fire near the center
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_fire, 1 );
        }
    }

    if( effects.count( "NAPALM_BIG" ) > 0 ) {
        g->explosion( p, 24, 0.8, true );
        // More intense fire near the center
        for( auto &pt : g->m.points_in_radius( p, 3, 0 ) ) {
            g->m.add_field( pt, fd_fire, 1 );
        }
    }

    if( effects.count( "MININUKE_MOD" ) > 0 ) {
        g->explosion( p, 450 );
        for( auto &pt : g->m.points_in_radius( p, 6, 0 ) ) {
            if( g->m.sees( p, pt, 3 ) &&
                g->m.passable( pt ) ) {
                g->m.add_field( pt, fd_nuke_gas, 3 );
            }
        }
    }

    if( effects.count( "ACIDBOMB" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_acid, 3 );
        }
    }


    if( effects.count( "EXPLOSIVE_BIG" ) > 0 ) {
        g->explosion( p, 40 );
    }

    if( effects.count( "EXPLOSIVE_HUGE" ) > 0 ) {
        g->explosion( p, 80 );
    }

    if( effects.count( "TOXICGAS" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_toxic_gas, 3 );
        }
    }
    if( effects.count( "GAS_FUNGICIDAL" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_fungicidal_gas, 3 );
        }
    }
    if( effects.count( "SMOKE" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_smoke, MAX_FIELD_DENSITY );
        }
    }
    if( effects.count( "SMOKE_BIG" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 6, 0 ) ) {
            g->m.add_field( pt, fd_smoke, MAX_FIELD_DENSITY );
        }
    }

    if( effects.count( "FLASHBANG" ) ) {
        g->flashbang( p );
    }

    if( effects.count( "EMP" ) ) {
        g->emp_blast( p );
    }

    if( effects.count( "NO_BOOM" ) == 0 && effects.count( "FLAME" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_fire, 1 );
        }
    }

    if( effects.count( "FLARE" ) > 0 ) {
        g->m.add_field( p, fd_fire, 1 );
    }

    if( effects.count( "LIGHTNING" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_electricity, 3 );
        }
    }

    if( effects.count( "PLASMA" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            if( one_in( 2 ) ) {
                g->m.add_field( pt, fd_plasma, rng( 2, 3 ) );
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

