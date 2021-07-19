#include "mapgendata.h"

#include "debug.h"
#include "map.h"
#include "mapdata.h"
#include "omdata.h"
#include "overmapbuffer.h"
#include "point.h"
#include "regional_settings.h"

mapgendata::mapgendata( oter_id north, oter_id east, oter_id south, oter_id west,
                        oter_id northeast, oter_id southeast, oter_id southwest, oter_id northwest,
                        oter_id up, oter_id down, int z, const regional_settings &rsettings, map &mp,
                        const oter_id &terrain_type, const float density, const time_point &when,
                        ::mission *const miss )
    : terrain_type_( terrain_type ), density_( density ), when_( when ), mission_( miss ), zlevel_( z )
    , t_nesw{ north, east, south, west, northeast, southeast, southwest, northwest }
    , t_above( up )
    , t_below( down )
    , region( rsettings )
    , m( mp )
    , default_groundcover( region.default_groundcover )
{
}

mapgendata::mapgendata( const tripoint_abs_omt &over, map &m, const float density,
                        const time_point &when, ::mission *const miss )
// NOLINTNEXTLINE( cata-unsequenced-calls )
    : mapgendata( overmap_buffer.ter( over + tripoint_north ),
                  overmap_buffer.ter( over + tripoint_east ),
                  overmap_buffer.ter( over + tripoint_south ),
                  overmap_buffer.ter( over + tripoint_west ),
                  overmap_buffer.ter( over + tripoint_north_east ),
                  overmap_buffer.ter( over + tripoint_south_east ),
                  overmap_buffer.ter( over + tripoint_south_west ),
                  overmap_buffer.ter( over + tripoint_north_west ),
                  overmap_buffer.ter( over + tripoint_above ),
                  overmap_buffer.ter( over + tripoint_below ),
                  over.z(), overmap_buffer.get_settings( over ), m,
                  overmap_buffer.ter( over ), density, when, miss )
{
}

mapgendata::mapgendata( const mapgendata &other, const oter_id &other_id ) : mapgendata( other )
{
    terrain_type_ = other_id;
}

mapgendata::mapgendata( const mapgendata &other,
                        const std::unordered_map<std::string, cata_variant> &mapgen_params ) :
    mapgendata( other )
{
    mapgen_params_ = mapgen_params;
}

void mapgendata::set_dir( int dir_in, int val )
{
    switch( dir_in ) {
        case 0:
            n_fac = val;
            break;
        case 1:
            e_fac = val;
            break;
        case 2:
            s_fac = val;
            break;
        case 3:
            w_fac = val;
            break;
        case 4:
            ne_fac = val;
            break;
        case 5:
            se_fac = val;
            break;
        case 6:
            sw_fac = val;
            break;
        case 7:
            nw_fac = val;
            break;
        default:
            debugmsg( "Invalid direction for mapgendata::set_dir.  dir_in = %d", dir_in );
            break;
    }
}

void mapgendata::fill( int val )
{
    n_fac = val;
    e_fac = val;
    s_fac = val;
    w_fac = val;
    ne_fac = val;
    se_fac = val;
    sw_fac = val;
    nw_fac = val;
}

int &mapgendata::dir( int dir_in )
{
    switch( dir_in ) {
        case 0:
            return n_fac;
        case 1:
            return e_fac;
        case 2:
            return s_fac;
        case 3:
            return w_fac;
        case 4:
            return ne_fac;
        case 5:
            return se_fac;
        case 6:
            return sw_fac;
        case 7:
            return nw_fac;
        default:
            debugmsg( "Invalid direction for mapgendata::set_dir.  dir_in = %d", dir_in );
            //return something just so the compiler doesn't freak out. Not really correct, though.
            return n_fac;
    }
}

void mapgendata::square_groundcover( const point &p1, const point &p2 ) const
{
    m.draw_square_ter( default_groundcover, p1, p2 );
}

void mapgendata::fill_groundcover() const
{
    m.draw_fill_background( default_groundcover );
}

bool mapgendata::is_groundcover( const ter_id &iid ) const
{
    for( const auto &pr : default_groundcover ) {
        if( pr.obj == iid ) {
            return true;
        }
    }

    return false;
}

ter_id mapgendata::groundcover() const
{
    const ter_id *tid = default_groundcover.pick();
    return tid != nullptr ? *tid : t_null;
}

const oter_id &mapgendata::neighbor_at( om_direction::type dir ) const
{
    // TODO: De-uglify, implement proper conversion somewhere
    switch( dir ) {
        case om_direction::type::north:
            return north();
        case om_direction::type::east:
            return east();
        case om_direction::type::south:
            return south();
        case om_direction::type::west:
            return west();
        default:
            break;
    }

    debugmsg( "Tried to get neighbor from invalid direction %d", dir );
    return north();
}
