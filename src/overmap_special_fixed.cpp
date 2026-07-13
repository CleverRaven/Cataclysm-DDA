#include "omdata.h" // IWYU pragma: associated

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <set>
#include <vector>

#include "basecamp.h"
#include "city.h"
#include "coordinates.h"
#include "debug.h"
#include "line.h"
#include "map.h"
#include "mapgen_functions.h"
#include "overmap.h"
#include "overmap_connection.h"
#include "overmapbuffer.h"
#include "rng.h"

void fixed_overmap_special_data::finalize(
    const std::string &,
    const cata::flat_set<string_id<overmap_location>> &default_locations )
{
    // If the special has default locations, then add those to the locations
    // of each of the terrains IF the terrain has no locations already.
    for( overmap_special_terrain &t : terrains ) {
        if( t.locations.empty() ) {
            t.locations = default_locations;
        }
    }

    for( overmap_special_connection &elem : connections ) {
        const overmap_special_terrain &oter = get_terrain_at( elem.p );
        if( !elem.terrain && oter.terrain ) {
            elem.terrain = oter.terrain->get_type_id();    // Defaulted.
        }

        // If the connection type hasn't been specified, we'll guess for them.
        // The guess isn't always right (hence guessing) in the case where
        // multiple connections types can be made on a single location type,
        // e.g. both roads and forest trails can be placed on "forest" locations.
        if( elem.connection.is_null() ) {
            elem.connection = overmap_connections::guess_for( elem.terrain );
        }

        // If the connection has a "from" hint specified, then figure out what the
        // resulting direction from the hinted location to the connection point is,
        // and use that as the initial direction to be passed off to the connection
        // building code.
        if( elem.from ) {
            const direction calculated_direction = direction_from( *elem.from, elem.p );
            switch( calculated_direction ) {
                case direction::NORTH:
                    elem.initial_dir = cube_direction::north;
                    break;
                case direction::EAST:
                    elem.initial_dir = cube_direction::east;
                    break;
                case direction::SOUTH:
                    elem.initial_dir = cube_direction::south;
                    break;
                case direction::WEST:
                    elem.initial_dir = cube_direction::west;
                    break;
                default:
                    // The only supported directions are north/east/south/west
                    // as those are the four directions that overmap connections
                    // can be made in. If the direction we figured out wasn't
                    // one of those, just set this as invalid. We'll provide
                    // a warning to the user/developer in overmap_special::check().
                    elem.initial_dir = cube_direction::last;
                    break;
            }
        }
    }
}

void fixed_overmap_special_data::finalize_mapgen_parameters( mapgen_parameters &params,
        const std::string &context ) const
{
    for( const overmap_special_terrain &t : terrains ) {
        if( !t.terrain.is_valid() ) {
            if( oter_str_id( t.terrain.str() + "_north" ).is_valid() ) {
                debugmsg( "In %s, terrain \"%s\" rotates, but is specified without a "
                          "rotation.", context, t.terrain.str() );
            } else {
                debugmsg( "In %s, terrain \"%s\" is invalid.", context, t.terrain.str() );
            }
        }
        std::string mapgen_id = t.terrain->get_mapgen_id();
        params.check_and_merge( get_map_special_params( mapgen_id ), context );
    }
}

void fixed_overmap_special_data::check( const std::string &context ) const
{
    std::set<oter_str_id> invalid_terrains;
    std::set<tripoint_rel_omt> points;

    for( const overmap_special_terrain &elem : terrains ) {
        const oter_str_id &oter = elem.terrain;

        if( !oter.is_valid() ) {
            if( !invalid_terrains.count( oter ) ) {
                // Not a huge fan of the direct id manipulation here, but I don't know
                // how else to do this
                // Because we try to access all the terrains in the finalization,
                // this is a little redundant, but whatever
                oter_str_id invalid( oter.str() + "_north" );
                if( invalid.is_valid() ) {
                    debugmsg( "In %s, terrain \"%s\" rotates, but is specified without a "
                              "rotation.", context, oter.str() );
                } else  {
                    debugmsg( "In %s, terrain \"%s\" is invalid.", context, oter.str() );
                }
                invalid_terrains.insert( oter );
            }
        }

        const tripoint_rel_omt &pos = elem.p;

        if( points.count( pos ) > 0 ) {
            debugmsg( "In %s, point %s is duplicated.", context, pos.to_string() );
        } else {
            points.insert( pos );
        }

        if( elem.camp_owner.has_value() ) {
            if( !elem.camp_owner.value().is_valid() ) {
                debugmsg( "In %s, camp at %s has invalid owner %s", context, pos.to_string(),
                          elem.camp_owner.value().c_str() );
            }
            if( elem.camp_name.empty() ) {
                debugmsg( "In %s, camp was defined but missing a camp_name.", context );
            }
        } else if( !elem.camp_name.empty() ) {
            debugmsg( "In %s, camp_name defined but no owner.  Invalid name is discarded.", context );
        }

        if( elem.locations.empty() ) {
            debugmsg( "In %s, no location is defined for point %s or the "
                      "overall special.", context, pos.to_string() );
        }

        for( const auto &l : elem.locations ) {
            if( !l.is_valid() ) {
                debugmsg( "In %s, point %s, location \"%s\" is invalid.",
                          context, pos.to_string(), l.c_str() );
            }
        }
    }

    for( const overmap_special_connection &elem : connections ) {
        const overmap_special_terrain &oter = get_terrain_at( elem.p );
        if( !elem.terrain ) {
            debugmsg( "In %s, connection %s doesn't have a terrain.",
                      context, elem.p.to_string() );
        } else if( !elem.existing && !elem.terrain->has_flag( oter_flags::line_drawing ) ) {
            debugmsg( "In %s, connection %s \"%s\" isn't drawn with lines.",
                      context, elem.p.to_string(), elem.terrain.c_str() );
        } else if( !elem.existing && oter.terrain && !oter.terrain->type_is( elem.terrain ) ) {
            debugmsg( "In %s, connection %s overwrites \"%s\".",
                      context, elem.p.to_string(), oter.terrain.c_str() );
        }

        if( elem.from ) {
            // The only supported directions are north/east/south/west
            // as those are the four directions that overmap connections
            // can be made in. If the direction we figured out wasn't
            // one of those, warn the user/developer.
            const direction calculated_direction = direction_from( *elem.from, elem.p );
            switch( calculated_direction ) {
                case direction::NORTH:
                case direction::EAST:
                case direction::SOUTH:
                case direction::WEST:
                    continue;
                default:
                    debugmsg( "In %s, connection %s is not directly north, "
                              "east, south or west of the defined \"from\" %s.",
                              context, elem.p.to_string(), elem.from->to_string() );
                    break;
            }
        }
    }
}

const overmap_special_terrain &fixed_overmap_special_data::get_terrain_at(
    const tripoint_rel_omt &p ) const
{
    const auto iter = std::find_if( terrains.begin(), terrains.end(),
    [ &p ]( const overmap_special_terrain & elem ) {
        return elem.p == p;
    } );
    if( iter == terrains.end() ) {
        static const overmap_special_terrain null_terrain{};
        return null_terrain;
    }
    return *iter;
}

std::vector<overmap_special_terrain> fixed_overmap_special_data::get_terrains() const
{
    return terrains;
}

std::vector<overmap_special_terrain> fixed_overmap_special_data::preview_terrains() const
{
    std::vector<overmap_special_terrain> result;
    std::copy_if( terrains.begin(), terrains.end(), std::back_inserter( result ),
    []( const overmap_special_terrain & terrain ) {
        return terrain.p.z() == 0;
    } );
    return result;
}

std::vector<overmap_special_locations> fixed_overmap_special_data::required_locations() const
{
    std::vector<overmap_special_locations> result;
    result.reserve( terrains.size() );
    std::copy( terrains.begin(), terrains.end(), std::back_inserter( result ) );
    return result;
}

int fixed_overmap_special_data::score_rotation_at( const overmap &om, const tripoint_om_omt &p,
        om_direction::type r ) const
{
    int score = 0;
    for( const overmap_special_connection &con : connections ) {
        const tripoint_om_omt rp = p + om_direction::rotate( con.p, r );
        if( !overmap::inbounds( rp ) ) {
            return -1;
        }
        const oter_id &oter = om.ter( rp );

        if( ( oter->get_type_id() == oter_type_str_id( con.terrain.str() ) ) ) {
            ++score; // Found another one satisfied connection.
        } else if( !oter || con.existing || !con.connection->pick_subtype_for( oter ) ) {
            return -1;
        }
    }
    return score;
}

special_placement_result fixed_overmap_special_data::place(
    overmap &om, const tripoint_om_omt &origin, om_direction::type dir, bool blob,
    const city &cit, bool must_be_unexplored ) const
{
    special_placement_result result;

    for( const overmap_special_terrain &elem : terrains ) {
        const tripoint_om_omt location = origin + om_direction::rotate( elem.p, dir );
        if( !( elem.terrain == oter_str_id::NULL_ID() ) ) {
            result.omts_used.push_back( location );
            const oter_id tid = elem.terrain->get_rotated( dir );
            om.ter_set( location, tid );
            if( elem.camp_owner.has_value() ) {
                // This always results in z=0, but pos() doesn't return z-level information...
                tripoint_abs_omt camp_loc =  {project_combine( om.pos(), location.xy() ), 0};
                get_map().add_camp( camp_loc, "faction_camp", false );
                std::optional<basecamp *> bcp = overmap_buffer.find_camp( camp_loc.xy() );
                if( !bcp ) {
                    debugmsg( "Camp placement during special generation failed at %s", camp_loc.to_string() );
                } else {
                    basecamp *temp_camp = *bcp;
                    temp_camp->set_owner( elem.camp_owner.value() );
                    temp_camp->set_name( elem.camp_name.translated() );
                    // FIXME? Camp types are raw strings! Not ideal.
                    temp_camp->define_camp( camp_loc, "faction_base_bare_bones_NPC_camp_0", false );
                }
            }
            if( blob ) {
                for( int x = -2; x <= 2; x++ ) {
                    for( int y = -2; y <= 2; y++ ) {
                        const tripoint_om_omt nearby_pos = location + point( x, y );
                        if( !overmap::inbounds( nearby_pos ) ) {
                            continue;
                        }
                        if( one_in( 1 + std::abs( x ) + std::abs( y ) ) &&
                            elem.can_be_placed_on( om.ter( nearby_pos ) ) ) {
                            om.ter_set( nearby_pos, tid );
                        }
                    }
                }
            }
        }
    }
    // Make connections.
    for( const overmap_special_connection &elem : connections ) {
        if( elem.connection ) {
            const tripoint_om_omt rp = origin + om_direction::rotate( elem.p, dir );
            cube_direction initial_dir = elem.initial_dir;

            if( initial_dir != cube_direction::last ) {
                initial_dir = initial_dir + dir;
            }
            // TODO: JSONification of logic + don't treat non roads like roads
            point_om_omt target;
            if( cit ) {
                target = cit.pos;
            } else {
                target = om.get_fallback_road_connection_point();
            }
            om.build_connection( target, rp.xy(), elem.p.z(), *elem.connection, must_be_unexplored,
                                 initial_dir );
        }
    }

    return result;
}
