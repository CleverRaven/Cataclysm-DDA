#include "projectile.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "ammo_effect.h"
#include "explosion.h"
#include "item.h"
#include "map.h"
#include "map_iterator.h"
#include "rng.h"
#include "string_id.h"

projectile::projectile() :
    speed( 0 ), range( 0 ), critical_multiplier( 2.0 ), drop( nullptr ), custom_explosion( nullptr )
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
    critical_multiplier = other.critical_multiplier;
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
    map &here = get_map();
    for( const ammo_effect &ae : ammo_effects::get_all() ) {
        if( effects.count( ae.id.str() ) > 0 ) {
            for( auto &pt : here.points_in_radius( p, ae.aoe_radius, ae.aoe_radius_z ) ) {
                if( x_in_y( ae.aoe_chance, 100 ) ) {
                    const bool check_sees = !ae.aoe_check_sees || here.sees( p, pt, ae.aoe_check_sees_radius );
                    const bool check_passable = !ae.aoe_check_passable || here.passable( pt );
                    if( check_sees && check_passable ) {
                        here.add_field( pt, ae.aoe_field_type, rng( ae.aoe_intensity_min, ae.aoe_intensity_max ) );
                    }
                }
            }
            if( ae.aoe_explosion_data.power > 0 ) {
                explosion_handler::explosion( p, ae.aoe_explosion_data );
            }
            if( ae.do_flashbang ) {
                explosion_handler::flashbang( p );
            }
            if( ae.do_emp_blast ) {
                explosion_handler::emp_blast( p );
            }
        }
    }
}

int max_aoe_size( const std::set<std::string> &tags )
{
    int aoe_size = 0;
    for( const ammo_effect &aed : ammo_effects::get_all() ) {
        if( tags.count( aed.id.str() ) > 0 ) {
            aoe_size = std::max( aoe_size,  aed.aoe_size ) ;
        }
    }
    return aoe_size;
}
