#include <algorithm>
#include <functional>
#include <iosfwd>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cata_assert.h"
#include "character.h"
#include "city.h"
#include "coordinates.h"
#include "condition.h"
#include "debug.h"
#include "dialogue.h"
#include "enum_conversions.h"
#include "enums.h"
#include "game.h"
#include "json.h"
#include "map_iterator.h"
#include "mapgen_functions.h"
#include "messages.h"
#include "mission.h"
#include "npc.h"
#include "omdata.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "rng.h"
#include "talker.h"
#include "translations.h"
#include "type_id.h"

static tripoint_abs_omt reveal_destination( const std::string &type )
{
    const tripoint_abs_omt your_pos = get_player_character().global_omt_location();
    const tripoint_abs_omt center_pos =
        overmap_buffer.find_random( your_pos, type, rng( 40, 80 ), false );

    if( center_pos != overmap::invalid_tripoint ) {
        overmap_buffer.reveal( center_pos, 2 );
        return center_pos;
    }

    return overmap::invalid_tripoint;
}

static void reveal_route( mission *miss, const tripoint_abs_omt &destination )
{
    const npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "couldn't find an NPC!" );
        return;
    }

    const tripoint_abs_omt source = get_player_character().global_omt_location();

    const tripoint_abs_omt source_road = overmap_buffer.find_closest( source, "road", 3, false );
    const tripoint_abs_omt dest_road = overmap_buffer.find_closest( destination, "road", 3, false );

    if( overmap_buffer.reveal_route( source_road, dest_road ) ) {
        add_msg( _( "%s also marks the road that leads to it…" ), p->get_name() );
    }
}

static void reveal_target( mission *miss, const std::string &omter_id )
{
    const npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "couldn't find an NPC!" );
        return;
    }

    const tripoint_abs_omt destination = reveal_destination( omter_id );
    if( destination != overmap::invalid_tripoint ) {
        const oter_id oter = overmap_buffer.ter( destination );
        add_msg( _( "%s has marked the only %s known to them on your map." ), p->get_name(),
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

bool mission_util::reveal_road( const tripoint_abs_omt &source, const tripoint_abs_omt &dest,
                                overmapbuffer &omb )
{
    const tripoint_abs_omt source_road = overmap_buffer.find_closest( source, "road", 3, false );
    const tripoint_abs_omt dest_road = overmap_buffer.find_closest( dest, "road", 3, false );
    return omb.reveal_route( source_road, dest_road );
}

/**
 * Reveal the cloest overmap terrain of a type and return the its location
 */
tripoint_abs_omt mission_util::reveal_om_ter( const std::string &omter, int reveal_rad,
        bool must_see, int target_z )
{
    // Missions are normally on z-level 0, but allow an optional argument.
    tripoint_abs_omt loc = get_player_character().global_omt_location();
    loc.z() = target_z;
    const tripoint_abs_omt place = overmap_buffer.find_closest( loc, omter, 0, must_see );
    if( place != overmap::invalid_tripoint && reveal_rad >= 0 ) {
        overmap_buffer.reveal( place, reveal_rad );
    }
    return place;
}

/**
 * Given a (valid!) city reference, select a random house within the city borders.
 * @return global overmap terrain coordinates of the house.
 */
static tripoint_abs_omt random_house_in_city( const city_reference &cref )
{
    // TODO: fix point types
    const tripoint_abs_omt city_center_omt =
        project_to<coords::omt>( cref.abs_sm_pos );
    std::vector<tripoint_abs_omt> valid;
    for( const tripoint_abs_omt &p : points_in_radius( city_center_omt, cref.city->size ) ) {
        if( overmap_buffer.check_ot( "house", ot_match_type::prefix, p ) ) {
            valid.push_back( p );
        }
    }
    return random_entry( valid, city_center_omt ); // center of the city is a good fallback
}

tripoint_abs_omt mission_util::random_house_in_closest_city()
{
    Character &player_character = get_player_character();
    const tripoint_abs_sm center = player_character.global_sm_location();
    const city_reference cref = overmap_buffer.closest_city( center );
    if( !cref ) {
        debugmsg( "could not find closest city" );
        return player_character.global_omt_location();
    }
    return random_house_in_city( cref );
}

tripoint_abs_omt mission_util::target_closest_lab_entrance(
    const tripoint_abs_omt &origin, int reveal_rad, mission *miss )
{
    tripoint_abs_omt testpoint = origin;
    // Get the surface locations for labs and for spaces above hidden lab stairs.
    testpoint.z() = 0;
    tripoint_abs_omt surface = overmap_buffer.find_closest( testpoint, "lab_stairs", 0, false,
                               ot_match_type::contains );

    testpoint.z() = -1;
    tripoint_abs_omt underground =
        overmap_buffer.find_closest( testpoint, "hidden_lab_stairs", 0, false,
                                     ot_match_type::contains );
    underground.z() = 0;

    tripoint_abs_omt closest;
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

static std::optional<tripoint_abs_omt> find_or_create_om_terrain(
    const tripoint_abs_omt &origin_pos, const mission_target_params &params )
{
    tripoint_abs_omt target_pos = overmap::invalid_tripoint;
    dialogue d( get_talker_for( get_avatar() ), nullptr );

    if( params.target_var.has_value() ) {
        return project_to<coords::omt>( get_tripoint_from_var( params.target_var.value(), d ) );
    }

    omt_find_params find_params;
    std::vector<std::pair<std::string, ot_match_type>> temp_types;
    std::pair<std::string, ot_match_type> temp_pair;
    temp_pair.first = params.overmap_terrain.evaluate( d );
    temp_pair.second = params.overmap_terrain_match_type;
    temp_types.push_back( temp_pair );
    find_params.types = temp_types;
    find_params.search_range = params.search_range.evaluate( d );
    find_params.min_distance = params.min_distance.evaluate( d );
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
                const bool placed = overmap_buffer.place_special( static_cast<overmap_special_id >(
                                        ( *params.overmap_special ).evaluate( d ) ), origin_pos,
                                    params.search_range.evaluate( d ) );
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
                find_params.types.front().first = ( *params.replaceable_overmap_terrain ).evaluate( d );
                target_pos = overmap_buffer.find_random( origin_pos, find_params );

                // We didn't find it, so allow this search to create new overmaps and try again.
                find_params.existing_only = true;
                if( target_pos == overmap::invalid_tripoint ) {
                    target_pos = overmap_buffer.find_random( origin_pos, find_params );
                }

                // We found a match, so set this position (which was our replacement terrain)
                // to our desired mission terrain.
                if( target_pos != overmap::invalid_tripoint ) {
                    overmap_buffer.ter_set( target_pos, oter_id( params.overmap_terrain.evaluate( d ) ) );
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
        debugmsg( "Unable to find and assign mission target %s.", params.overmap_terrain.evaluate( d ) );
        return std::nullopt;
    }
    return target_pos;
}

static tripoint_abs_omt get_mission_om_origin( const mission_target_params &params )
{
    // use the player or NPC's current position, adjust for the z value if any
    tripoint_abs_omt origin_pos = get_player_character().global_omt_location();
    dialogue d( get_talker_for( get_avatar() ), nullptr );
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
        origin_pos.z() = ( *params.z ).evaluate( d );
    }
    return origin_pos;
}

std::optional<tripoint_abs_omt> mission_util::assign_mission_target(
    const mission_target_params &params )
{
    // use the player or NPC's current position, adjust for the z value if any
    tripoint_abs_omt origin_pos = get_mission_om_origin( params );

    std::optional<tripoint_abs_omt> target_pos = find_or_create_om_terrain( origin_pos, params );
    dialogue d( get_talker_for( get_avatar() ), nullptr );
    if( target_pos ) {
        if( params.offset ) {
            *target_pos += *params.offset;
        }

        // If we specified a reveal radius, then go ahead and reveal around our found position.
        if( params.reveal_radius ) {
            overmap_buffer.reveal( *target_pos, ( *params.reveal_radius ).evaluate( d ) );
        }

        // Set the mission target to our found position.
        params.mission_pointer->set_target( *target_pos );
    }

    return target_pos;
}

tripoint_abs_omt mission_util::get_om_terrain_pos( const mission_target_params &params )
{
    // use the player or NPC's current position, adjust for the z value if any
    tripoint_abs_omt origin_pos = get_mission_om_origin( params );
    dialogue d( get_talker_for( get_avatar() ), nullptr );
    tripoint_abs_omt target_pos = origin_pos;
    if( ( params.overmap_terrain.default_val.has_value() ||
          params.overmap_terrain.str_val.has_value() || params.overmap_terrain.var_val.has_value() ) &&
        !params.overmap_terrain.evaluate( d ).empty() ) {
        std::optional<tripoint_abs_omt> temp_pos = find_or_create_om_terrain( origin_pos, params );
        if( temp_pos ) {
            target_pos = *temp_pos;
        }
    }

    if( params.offset ) {
        target_pos += *params.offset;
    }

    // If we specified a reveal radius, then go ahead and reveal around our found position.
    if( params.reveal_radius ) {
        overmap_buffer.reveal( target_pos, ( *params.reveal_radius ).evaluate( d ) );
    }

    return target_pos;
}

/**
 * Set target of mission to closest overmap terrain of that type,
 * reveal the area around it (uses reveal with reveal_rad),
 * and returns the mission target.
*/
tripoint_abs_omt mission_util::target_om_ter(
    const std::string &omter, int reveal_rad, mission *miss, bool must_see, int target_z )
{
    const tripoint_abs_omt place = reveal_om_ter( omter, reveal_rad, must_see, target_z );
    miss->set_target( place );
    return place;
}

tripoint_abs_omt mission_util::target_om_ter_random( const std::string &omter, int reveal_rad,
        mission *miss, bool must_see, int range, tripoint_abs_omt loc )
{
    Character &player_character = get_player_character();
    if( loc == overmap::invalid_tripoint ) {
        loc = player_character.global_omt_location();
    }

    auto places = overmap_buffer.find_all( loc, omter, range, must_see );
    if( places.empty() ) {
        return player_character.global_omt_location();
    }
    const overmap *loc_om = overmap_buffer.get_existing_om_global( loc ).om;
    cata_assert( loc_om );

    std::vector<tripoint_abs_omt> places_om;
    for( auto &i : places ) {
        if( loc_om == overmap_buffer.get_existing_om_global( i ).om ) {
            places_om.push_back( i );
        }
    }

    const tripoint_abs_omt place = random_entry( places_om );
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
        p.overmap_terrain = get_str_or_var( jo.get_member( "om_terrain" ), "om_terrain", true );
    }
    if( jo.has_member( "om_terrain_match_type" ) ) {
        p.overmap_terrain_match_type = jo.get_enum_value<ot_match_type>( "om_terrain_match_type" );
    }
    if( jo.get_bool( "origin_npc", false ) ) {
        p.origin_u = false;
    }
    if( jo.has_member( "om_terrain_replace" ) ) {
        p.replaceable_overmap_terrain = get_str_or_var( jo.get_member( "om_terrain_replace" ),
                                        "om_terrain_replace", true );
    }
    if( jo.has_member( "om_special" ) ) {
        p.overmap_special = get_str_or_var( jo.get_member( "om_special" ), "om_special", true );
    }
    if( jo.has_member( "reveal_radius" ) ) {
        p.reveal_radius = get_dbl_or_var( jo, "reveal_radius" );
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
    p.search_range  = get_dbl_or_var( jo, "search_range", false, OMAPX );
    p.min_distance  = get_dbl_or_var( jo, "min_distance", false );

    if( jo.has_member( "offset_x" ) || jo.has_member( "offset_y" ) || jo.has_member( "offset_z" ) ) {
        tripoint_rel_omt offset;
        if( jo.has_member( "offset_x" ) ) {
            offset.x() = jo.get_int( "offset_x" );
        }
        if( jo.has_member( "offset_y" ) ) {
            offset.y() = jo.get_int( "offset_y" );
        }
        if( jo.has_member( "offset_z" ) ) {
            offset.z() = jo.get_int( "offset_z" );
        }
        p.offset = offset;
    }
    if( jo.has_member( "z" ) ) {
        p.z = get_dbl_or_var( jo, "z" );
    }
    if( jo.has_member( "var" ) ) {
        p.target_var = read_var_info( jo.get_object( "var" ) );
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
            tripoint_abs_omt update_pos3 = mission_util::reveal_om_ter( om_terrain, 1, false );
            update_map( update_pos3, miss );
        };
        funcs.emplace_back( mission_func );
    } else {
        const auto mission_func = [update_map]( mission * miss ) {
            tripoint_abs_omt update_pos3 = miss->get_target();
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
    talk_effects.load_effect( jo, "effect" );
    phase_func = [ funcs, talk_effects ]( mission * miss ) {
        npc *beta_npc = g->find_npc( miss->get_npc_id() );
        ::dialogue d( get_talker_for( get_avatar() ),
                      beta_npc == nullptr ? nullptr : get_talker_for( beta_npc ) );
        for( const talk_effect_fun_t &effect : talk_effects.effects ) {
            effect( d );
        }
        for( const auto &mission_function : funcs ) {
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
