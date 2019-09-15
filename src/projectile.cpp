#include "projectile.h"

#include <memory>
#include <utility>

#include "explosion.h"
#include "field.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_iterator.h"
#include "rng.h"

projectile::projectile() :
    speed( 0 ), range( 0 ), drop( nullptr ), custom_explosion( nullptr )
{ }

projectile::~projectile() = default;

projectile::projectile( projectile && ) = default;

projectile::projectile( const projectile &other )
{
    *this = other;
}

projectile &projectile::operator=( const projectile &other )
{
    impact = other.impact;
    speed = other.speed;
    range = other.range;
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
        drop = std::make_unique<item>( it );
    }
}

void projectile::set_drop( item &&it )
{
    if( it.is_null() ) {
        unset_drop();
    } else {
        drop = std::make_unique<item>( std::move( it ) );
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
    custom_explosion = std::make_unique<explosion_data>( ex );
}

void projectile::unset_custom_explosion()
{
    custom_explosion.reset();
}

void apply_ammo_effects( const tripoint &p, const std::set<std::string> &effects )
{
    if( effects.count( "EXPLOSIVE_SMALL" ) > 0 ) {
        // TODO: double-check if this is sensible.
        explosion_handler::explosion( p, 360, 0.4 );
    }

    if( effects.count( "EXPLOSIVE" ) > 0 ) {
        // TODO: double-check if this is sensible.
        explosion_handler::explosion( p, 360 );
    }

    if( effects.count( "FRAG" ) > 0 ) {
        // Same as a standard thrown frag grenade.
        explosion_handler::explosion( p, 185, 0.8, false, 212, 0.05 );
    }

    if( effects.count( "NAPALM" ) > 0 ) {
        explosion_handler::explosion( p, 60, 0.7, true );
        // More intense fire near the center
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_fire, 1 );
        }
    }

    if( effects.count( "NAPALM_BIG" ) > 0 ) {
        explosion_handler::explosion( p, 360, 0.8, true );
        // More intense fire near the center
        for( auto &pt : g->m.points_in_radius( p, 3, 0 ) ) {
            g->m.add_field( pt, fd_fire, 1 );
        }
    }

    if( effects.count( "PYROPHORIC" ) > 0 ) {
        explosion_handler::explosion( p, 360, 0.8, true );
        // Extreme heat near the center of the explosion
        for( auto &pt : g->m.points_in_radius( p, 3, 0 ) ) {
            g->m.add_field( pt, fd_fire, 2 );
        }
    }

    if( effects.count( "MININUKE_MOD" ) > 0 ) {
        explosion_handler::explosion( p, 72000000 );
        for( auto &pt : g->m.points_in_radius( p, 18, 0 ) ) {
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
        // TODO: double-check if this is sensible.
        explosion_handler::explosion( p, 600 );
    }

    if( effects.count( "EXPLOSIVE_HUGE" ) > 0 ) {
        // TODO: double-check if this is sensible.
        explosion_handler::explosion( p, 1200 );
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
    if( effects.count( "GAS_INSECTICIDAL" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_insecticidal_gas, 3 );
        }
    }
    if( effects.count( "SMOKE" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 1, 0 ) ) {
            g->m.add_field( pt, fd_smoke, 3 );
        }
    }
    if( effects.count( "SMOKE_BIG" ) > 0 ) {
        for( auto &pt : g->m.points_in_radius( p, 6, 0 ) ) {
            g->m.add_field( pt, fd_smoke, 3 );
        }
    }

    if( effects.count( "FLASHBANG" ) ) {
        explosion_handler::flashbang( p );
    }

    if( effects.count( "EMP" ) ) {
        explosion_handler::emp_blast( p );
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
               tags.count( "EXPLOSIVE_SMALL" ) ) {
        return 2;
    } else if( tags.count( "FRAG" ) ) {
        return 15;
    } else if( tags.count( "ACIDBOMB" ) ||
               tags.count( "FLAME" ) ) {
        return 1;
    }

    return 0;
}

