#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "dialogue.h"
#include "enum_conversions.h"
#include "enums.h"
#include "game.h"
#include "json.h"
#include "line.h"
#include "map_iterator.h"
#include "mapgen_functions.h"
#include "messages.h"
#include "mission.h"
#include "npc.h"
#include "omdata.h"
#include "optional.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "rng.h"
#include "talker.h"
#include "translations.h"
#include "type_id.h"

static tripoint reveal_destination( const std::string &type )
{
    const tripoint your_pos = g->u.global_omt_location();
    const tripoint center_pos = overmap_buffer.find_random( your_pos, type, rng( 40, 80 ), false );

    if( center_pos != overmap::invalid_tripoint ) {
        overmap_buffer.reveal( center_pos, 2 );
        return center_pos;
    }

    return overmap::invalid_tripoint;
}

static void reveal_route( mission *miss, const tripoint &destination )
{
    const npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "couldn't find an NPC!" );
        return;
    }

    const tripoint source = g->u.global_omt_location();

    const tripoint source_road = overmap_buffer.find_closest( source, "road", 3, false );
    const tripoint dest_road = overmap_buffer.find_closest( destination, "road", 3, false );

    if( overmap_buffer.reveal_route( source_road, dest_road ) ) {
        add_msg( _( "%s also marks the road that leads to it…" ), p->name );
    }
}

static void reveal_target( mission *miss, const std::string &omter_id )
{
    const npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "couldn't find an NPC!" );
        return;
    }

    const tripoint destination = reveal_destination( omter_id );
    if( destination != overmap::invalid_tripoint ) {
        const oter_id oter = overmap_buffer.ter( destination );
        add_msg( _( "%s has marked the only %s known to them on your map." ), p->name,
                 oter->get_name() );
        miss->set_target( destination );
        if( one_in( 3 ) ) {
            reveal_route( miss, destination );
        }
    }
}

static void reveal_any_target( mission *miss, const std::vector<std::string> &omter_ids )
{
    reveal_target( miss, random_entry( omter_ids ) );
}

bool mission_util::reveal_road( const tripoint &source, const tripoint &dest, overmapbuffer &omb )
{
    const tripoint source_road = overmap_buffer.find_closest( source, "road", 3, false );
    const tripoint dest_road = overmap_buffer.find_closest( dest, "road", 3, false );
    return omb.reveal_route( source_road, dest_road );
}

/**
 * Reveal the cloest overmap terrain of a type and return the its location
 */
tripoint mission_util::reveal_om_ter( const std::string &omter, int reveal_rad, bool must_see,
                                      int target_z )
{
    // Missions are normally on z-level 0, but allow an optional argument.
    tripoint loc = g->u.global_omt_location();
    loc.z = target_z;
    const tripoint place = overmap_buffer.find_closest( loc, omter, 0, must_see );
    if( place != overmap::invalid_tripoint && reveal_rad >= 0 ) {
        overmap_buffer.reveal( place, reveal_rad );
    }
    return place;
}

/**
 * Given a (valid!) city reference, select a random house within the city borders.
 * @return global overmap terrain coordinates of the house.
 */
static tripoint random_house_in_city( const city_reference &cref )
{
    const tripoint city_center_omt = sm_to_omt_copy( cref.abs_sm_pos );
    std::vector<tripoint> valid;
    for( const tripoint &p : points_in_radius( city_center_omt, cref.city->size ) ) {
        if( overmap_buffer.check_ot( "house", ot_match_type::prefix, p ) ) {
            valid.push_back( p );
        }
    }
    return random_entry( valid, city_center_omt ); // center of the city is a good fallback
}

tripoint mission_util::random_house_in_closest_city()
{
    const auto center = g->u.global_sm_location();
    const auto cref = overmap_buffer.closest_city( center );
    if( !cref ) {
        debugmsg( "could not find closest city" );
        return g->u.global_omt_location();
    }
    return random_house_in_city( cref );
}

tripoint mission_util::target_closest_lab_entrance( const tripoint &origin, int reveal_rad,
        mission *miss )
{
    tripoint testpoint = tripoint( origin );
    // Get the surface locations for labs and for spaces above hidden lab stairs.
    testpoint.z = 0;
    tripoint surface = overmap_buffer.find_closest( testpoint, "lab_stairs", 0, false,
                       ot_match_type::contains );

    testpoint.z = -1;
    tripoint underground = overmap_buffer.find_closest( testpoint, "hidden_lab_stairs", 0, false,
                           ot_match_type::contains );
    underground.z = 0;

    tripoint closest;
    if( square_dist( surface.xy(), origin.xy() ) <= square_dist( underground.xy(), origin.xy() ) ) {
        closest = surface;
    } else {
        closest = underground;
    }

    if( closest != overmap::invalid_tripoint && reveal_rad >= 0 ) {
        overmap_buffer.reveal( closest, reveal_rad );
    }
    miss->set_target( closest );
    return closest;
}

static cata::optional<tripoint> find_or_create_om_terrain( const tripoint &origin_pos,
        const mission_target_params &params )
{
    tripoint target_pos = overmap::invalid_tripoint;

    omt_find_params find_params;
    std::vector<std::pair<std::string, ot_match_type>> temp_types;
    std::pair<std::string, ot_match_type> temp_pair;
    temp_pair.first = params.overmap_terrain;
    temp_pair.second = params.overmap_terrain_match_type;
    temp_types.push_back( temp_pair );
    find_params.types = temp_types;
    find_params.search_range = params.search_range;
    find_params.min_distance = params.min_distance;
    find_params.must_see = params.must_see;
    find_params.cant_see = params.cant_see;
    find_params.existing_only = true;

    auto get_target_position = [&]() {
        // Either find a random or closest match, based on the criteria.
        if( params.random ) {
            target_pos = overmap_buffer.find_random( origin_pos, find_params );
        } else {
            target_pos = overmap_buffer.find_closest( origin_pos, find_params );
        }

        // If we didn't find a match, and we're allowed to create new terrain, and the player didn't
        // have to see the location beforehand, then we can attempt to create the new terrain.
        if( target_pos == overmap::invalid_tripoint && params.create_if_necessary &&
            !params.must_see ) {
            // If this terrain is part of an overmap special...
            if( params.overmap_special ) {
                // ...then attempt to place the whole special.
                const bool placed = overmap_buffer.place_special( *params.overmap_special, origin_pos,
                                    params.search_range );
                // If we succeeded in placing the special, then try and find the particular location
                // we're interested in.
                if( placed ) {
                    find_params.must_see = false;
                    target_pos = overmap_buffer.find_closest( origin_pos, find_params );
                }
            } else if( params.replaceable_overmap_terrain ) {
                // This terrain wasn't part of an overmap special, but we do have a replacement
                // terrain specified. Find a random location of that replacement type.
                find_params.must_see = false;
                find_params.types.front().first = *params.replaceable_overmap_terrain;
                target_pos = overmap_buffer.find_random( origin_pos, find_params );

                // We didn't find it, so allow this search to create new overmaps and try again.
                find_params.existing_only = true;
                if( target_pos == overmap::invalid_tripoint ) {
                    target_pos = overmap_buffer.find_random( origin_pos, find_params );
                }

                // We found a match, so set this position (which was our replacement terrain)
                // to our desired mission terrain.
                if( target_pos != overmap::invalid_tripoint ) {
                    overmap_buffer.ter_set( target_pos, oter_id( params.overmap_terrain ) );
                }
            }
        }
    };

    // First try to get the position where we only allow existing overmaps.
    get_target_position();

    if( target_pos == overmap::invalid_tripoint ) {
        // If it's invalid, then that means we couldn't find it or create it (if allowed) on
        // our current overmap. We'll now go ahead and try again but allow it to create new overmaps.
        find_params.existing_only = false;
        get_target_position();
    }

    // If we got here and this is still invalid, it means that we couldn't find it nor create it
    // on any overmap (new or existing) within the allowed search range.
    if( target_pos == overmap::invalid_tripoint ) {
        debugmsg( "Unable to find and assign mission target %s.", params.overmap_terrain );
        return cata::nullopt;
    }
    return target_pos;
}

static tripoint get_mission_om_origin( const mission_target_params &params )
{
    // use the player or NPC's current position, adjust for the z value if any
    tripoint origin_pos = g->u.global_omt_location();
    if( !params.origin_u ) {
        npc *guy = nullptr;

        if( params.guy ) {
            guy = params.guy;
        } else if( params.mission_pointer ) {
            guy = g->find_npc( params.mission_pointer->get_npc_id() );
        }
        if( guy ) {
            origin_pos = guy->global_omt_location();
        }
    }
    if( params.z ) {
        origin_pos.z = *params.z;
    }
    return origin_pos;
}

cata::optional<tripoint> mission_util::assign_mission_target( const mission_target_params &params )
{
    // use the player or NPC's current position, adjust for the z value if any
    tripoint origin_pos = get_mission_om_origin( params );

    cata::optional<tripoint> target_pos = find_or_create_om_terrain( origin_pos, params );

    if( target_pos ) {
        if( params.offset ) {
            *target_pos += *params.offset;
        }

        // If we specified a reveal radius, then go ahead and reveal around our found position.
        if( params.reveal_radius ) {
            overmap_buffer.reveal( *target_pos, *params.reveal_radius );
        }

        // Set the mission target to our found position.
        params.mission_pointer->set_target( *target_pos );
    }

    return target_pos;
}

tripoint mission_util::get_om_terrain_pos( const mission_target_params &params )
{
    // use the player or NPC's current position, adjust for the z value if any
    tripoint origin_pos = get_mission_om_origin( params );

    tripoint target_pos = origin_pos;
    if( !params.overmap_terrain.empty() ) {
        cata::optional<tripoint> temp_pos = find_or_create_om_terrain( origin_pos, params );
        if( temp_pos ) {
            target_pos = *temp_pos;
        }
    }

    if( params.offset ) {
        target_pos += *params.offset;
    }

    // If we specified a reveal radius, then go ahead and reveal around our found position.
    if( params.reveal_radius ) {
        overmap_buffer.reveal( target_pos, *params.reveal_radius );
    }

    return target_pos;
}

/**
 * Set target of mission to closest overmap terrain of that type,
 * reveal the area around it (uses reveal with reveal_rad),
 * and returns the mission target.
*/
tripoint mission_util::target_om_ter( const std::string &omter, int reveal_rad, mission *miss,
                                      bool must_see, int target_z )
{
    const tripoint place = reveal_om_ter( omter, reveal_rad, must_see, target_z );
    miss->set_target( place );
    return place;
}

tripoint mission_util::target_om_ter_random( const std::string &omter, int reveal_rad,
        mission *miss, bool must_see, int range, tripoint loc )
{
    if( loc == overmap::invalid_tripoint ) {
        loc = g->u.global_omt_location();
    }

    auto places = overmap_buffer.find_all( loc, omter, range, must_see );
    if( places.empty() ) {
        return g->u.global_omt_location();
    }
    const overmap *loc_om = overmap_buffer.get_existing_om_global( loc ).om;
    assert( loc_om );

    std::vector<tripoint> places_om;
    for( auto &i : places ) {
        if( loc_om == overmap_buffer.get_existing_om_global( i ).om ) {
            places_om.push_back( i );
        }
    }

    const tripoint place = random_entry( places_om );
    if( reveal_rad >= 0 ) {
        overmap_buffer.reveal( place, reveal_rad );
    }
    miss->set_target( place );
    return place;
}

mission_target_params mission_util::parse_mission_om_target( const JsonObject &jo )
{
    mission_target_params p;
    if( jo.has_member( "om_terrain" ) ) {
        p.overmap_terrain = jo.get_string( "om_terrain" );
    }
    if( jo.has_member( "om_terrain_match_type" ) ) {
        p.overmap_terrain_match_type = jo.get_enum_value<ot_match_type>( "om_terrain_match_type" );
    }
    if( jo.get_bool( "origin_npc", false ) ) {
        p.origin_u = false;
    }
    if( jo.has_member( "om_terrain_replace" ) ) {
        p.replaceable_overmap_terrain = jo.get_string( "om_terrain_replace" );
    }
    if( jo.has_member( "om_special" ) ) {
        p.overmap_special = overmap_special_id( jo.get_string( "om_special" ) );
    }
    if( jo.has_member( "reveal_radius" ) ) {
        p.reveal_radius = std::max( 1, jo.get_int( "reveal_radius" ) );
    }
    if( jo.has_member( "must_see" ) ) {
        p.must_see = jo.get_bool( "must_see" );
    }
    if( jo.has_member( "cant_see" ) ) {
        p.cant_see = jo.get_bool( "cant_see" );
    }
    if( jo.has_member( "exclude_seen" ) ) {
        p.random = jo.get_bool( "exclude" );
    }
    if( jo.has_member( "random" ) ) {
        p.random = jo.get_bool( "random" );
    }
    if( jo.has_member( "search_range" ) ) {
        p.search_range = std::max( 1, jo.get_int( "search_range" ) );
    }
    if( jo.has_member( "min_distance" ) ) {
        p.min_distance = std::max( 1, jo.get_int( "min_distance" ) );
    }
    if( jo.has_member( "offset_x" ) || jo.has_member( "offset_y" ) || jo.has_member( "offset_z" ) ) {
        tripoint offset;
        if( jo.has_member( "offset_x" ) ) {
            offset.x = jo.get_int( "offset_x" );
        }
        if( jo.has_member( "offset_y" ) ) {
            offset.y = jo.get_int( "offset_y" );
        }
        if( jo.has_member( "offset_z" ) ) {
            offset.z = jo.get_int( "offset_z" );
        }
        p.offset = offset;
    }
    if( jo.has_member( "z" ) ) {
        p.z = jo.get_int( "z" );
    }
    return p;
}

void mission_util::set_reveal( const std::string &terrain,
                               std::vector<std::function<void( mission *miss )>> &funcs )
{
    const auto mission_func = [ terrain ]( mission * miss ) {
        reveal_target( miss, terrain );
    };
    funcs.emplace_back( mission_func );
}

void mission_util::set_reveal_any( const JsonArray &ja,
                                   std::vector<std::function<void( mission *miss )>> &funcs )
{
    std::vector<std::string> terrains;
    for( const std::string terrain : ja ) {
        terrains.push_back( terrain );
    }
    const auto mission_func = [ terrains ]( mission * miss ) {
        reveal_any_target( miss, terrains );
    };
    funcs.emplace_back( mission_func );
}

void mission_util::set_assign_om_target( const JsonObject &jo,
        std::vector<std::function<void( mission *miss )>> &funcs )
{
    mission_target_params p = parse_mission_om_target( jo );
    const auto mission_func = [p]( mission * miss ) {
        mission_target_params mtp = p;
        mtp.mission_pointer = miss;
        assign_mission_target( mtp );
    };
    funcs.emplace_back( mission_func );
}

bool mission_util::set_update_mapgen( const JsonObject &jo,
                                      std::vector<std::function<void( mission *miss )>> &funcs )
{
    bool defer = false;
    mapgen_update_func update_map = add_mapgen_update_func( jo, defer );
    if( defer ) {
        jo.allow_omitted_members();
        return false;
    }

    if( jo.has_member( "om_terrain" ) ) {
        const std::string om_terrain = jo.get_string( "om_terrain" );
        const auto mission_func = [update_map, om_terrain]( mission * miss ) {
            tripoint update_pos3 = mission_util::reveal_om_ter( om_terrain, 1, false );
            update_map( update_pos3, miss );
        };
        funcs.emplace_back( mission_func );
    } else {
        const auto mission_func = [update_map]( mission * miss ) {
            tripoint update_pos3 = miss->get_target();
            update_map( update_pos3, miss );
        };
        funcs.emplace_back( mission_func );
    }
    return true;
}

bool mission_util::load_funcs( const JsonObject &jo,
                               std::vector<std::function<void( mission *miss )>> &funcs )
{
    if( jo.has_string( "reveal_om_ter" ) ) {
        const std::string target_terrain = jo.get_string( "reveal_om_ter" );
        set_reveal( target_terrain, funcs );
    } else if( jo.has_member( "reveal_om_ter" ) ) {
        set_reveal_any( jo.get_array( "reveal_om_ter" ), funcs );
    } else if( jo.has_member( "assign_mission_target" ) ) {
        JsonObject mission_target = jo.get_object( "assign_mission_target" );
        set_assign_om_target( mission_target, funcs );
    }

    if( jo.has_object( "update_mapgen" ) ) {
        JsonObject update_mapgen = jo.get_object( "update_mapgen" );
        if( !set_update_mapgen( update_mapgen, funcs ) ) {
            return false;
        }
    } else {
        for( JsonObject update_mapgen : jo.get_array( "update_mapgen" ) ) {
            if( !set_update_mapgen( update_mapgen, funcs ) ) {
                return false;
            }
        }
    }

    return true;
}

bool mission_type::parse_funcs( const JsonObject &jo, std::function<void( mission * )> &phase_func )
{
    std::vector<std::function<void( mission *miss )>> funcs;
    if( !mission_util::load_funcs( jo, funcs ) ) {
        return false;
    }

    /* this is a kind of gross hijack of the dialogue responses effect system, but I don't want to
     * write that code in two places so here it goes.
     */
    talk_effect_t talk_effects;
    talk_effects.load_effect( jo );
    phase_func = [ funcs, talk_effects ]( mission * miss ) {
        ::dialogue d;
        npc *beta = g->find_npc( miss->get_npc_id() );
        standard_npc default_npc( "Default" );
        if( beta == nullptr ) {
            beta = &default_npc;
        }
        d.alpha = get_talker_for( g->u );
        d.beta = get_talker_for( beta );
        for( const talk_effect_fun_t &effect : talk_effects.effects ) {
            effect( d );
        }
        for( auto &mission_function : funcs ) {
            mission_function( miss );
        }
    };

    for( talk_effect_fun_t &effect : talk_effects.effects ) {
        auto rewards = effect.get_likely_rewards();
        if( !rewards.empty() ) {
            likely_rewards.insert( likely_rewards.end(), rewards.begin(), rewards.end() );
        }
    }

    return true;
}
