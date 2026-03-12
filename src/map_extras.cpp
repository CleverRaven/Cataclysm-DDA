#include "map_extras.h"

#include <array>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_utility.h"
#include "cellular_automata.h"
#include "character_id.h"
#include "coordinates.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enums.h"
#include "field_type.h"
#include "flexbuffer_json.h"
#include "fungal_effects.h"
#include "generic_factory.h"
#include "item_group.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "mapgen.h"
#include "mapgen_functions.h"
#include "mapgendata.h"
#include "omdata.h"
#include "overmapbuffer.h"
#include "point.h"
#include "regional_settings.h"
#include "ret_val.h"
#include "rng.h"
#include "sets_intersect.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_group.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weighted_list.h"

static const flag_id json_flag_FILTHY( "FILTHY" );

static const furn_str_id furn_f_barricade_road( "f_barricade_road" );
static const furn_str_id furn_f_beach_log( "f_beach_log" );
static const furn_str_id furn_f_beach_seaweed( "f_beach_seaweed" );
static const furn_str_id furn_f_boulder_large( "f_boulder_large" );
static const furn_str_id furn_f_boulder_medium( "f_boulder_medium" );
static const furn_str_id furn_f_boulder_small( "f_boulder_small" );
static const furn_str_id furn_f_cattails( "f_cattails" );
static const furn_str_id furn_f_crate_c( "f_crate_c" );
static const furn_str_id furn_f_lilypad( "f_lilypad" );
static const furn_str_id furn_f_lotus( "f_lotus" );
static const furn_str_id furn_f_wreckage( "f_wreckage" );

static const item_group_id Item_spawn_data_ammo_casings( "ammo_casings" );
static const item_group_id Item_spawn_data_everyday_corpse( "everyday_corpse" );
static const item_group_id Item_spawn_data_map_extra_casings( "map_extra_casings" );
static const item_group_id Item_spawn_data_mine_equipment( "mine_equipment" );
static const item_group_id Item_spawn_data_remains_human_generic( "remains_human_generic" );

static const itype_id itype_chunk_sulfur( "chunk_sulfur" );
static const itype_id itype_material_soil( "material_soil" );
static const itype_id itype_sheet_cotton( "sheet_cotton" );
static const itype_id itype_stick( "stick" );
static const itype_id itype_stick_long( "stick_long" );
static const itype_id itype_withered( "withered" );

static const map_extra_id map_extra_mx_casings( "mx_casings" );
static const map_extra_id map_extra_mx_corpses( "mx_corpses" );
static const map_extra_id map_extra_mx_fungal_zone( "mx_fungal_zone" );
static const map_extra_id map_extra_mx_grove( "mx_grove" );
static const map_extra_id map_extra_mx_helicopter( "mx_helicopter" );
static const map_extra_id map_extra_mx_looters( "mx_looters" );
static const map_extra_id map_extra_mx_null( "mx_null" );
static const map_extra_id map_extra_mx_pond( "mx_pond" );
static const map_extra_id map_extra_mx_portal_in( "mx_portal_in" );
static const map_extra_id map_extra_mx_reed( "mx_reed" );
static const map_extra_id map_extra_mx_roadworks( "mx_roadworks" );
static const map_extra_id map_extra_mx_sandy_beach( "mx_sandy_beach" );
static const map_extra_id map_extra_mx_shrubbery( "mx_shrubbery" );

static const mongroup_id GROUP_FISH( "GROUP_FISH" );
static const mongroup_id GROUP_FUNGI_FUNGALOID( "GROUP_FUNGI_FUNGALOID" );
static const mongroup_id GROUP_MIL_PASSENGER( "GROUP_MIL_PASSENGER" );
static const mongroup_id GROUP_MIL_PILOT( "GROUP_MIL_PILOT" );
static const mongroup_id GROUP_MIL_WEAK( "GROUP_MIL_WEAK" );
static const mongroup_id GROUP_NETHER_PORTAL( "GROUP_NETHER_PORTAL" );
static const mongroup_id GROUP_STRAY_DOGS( "GROUP_STRAY_DOGS" );

static const mtype_id mon_fungaloid_queen( "mon_fungaloid_queen" );

static const oter_type_str_id oter_type_road( "road" );

static const region_settings_id region_settings_default( "default" );

static const relic_procgen_id relic_procgen_data_alien_reality( "alien_reality" );

static const ter_str_id ter_t_coast_rock_surf( "t_coast_rock_surf" );
static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_dirtmound( "t_dirtmound" );
static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_grass_dead( "t_grass_dead" );
static const ter_str_id ter_t_grass_golf( "t_grass_golf" );
static const ter_str_id ter_t_grass_long( "t_grass_long" );
static const ter_str_id ter_t_grass_tall( "t_grass_tall" );
static const ter_str_id ter_t_grass_white( "t_grass_white" );
static const ter_str_id ter_t_lava( "t_lava" );
static const ter_str_id ter_t_moss( "t_moss" );
static const ter_str_id ter_t_pavement( "t_pavement" );
static const ter_str_id ter_t_pit( "t_pit" );
static const ter_str_id ter_t_pit_shallow( "t_pit_shallow" );
static const ter_str_id ter_t_sand( "t_sand" );
static const ter_str_id ter_t_stump( "t_stump" );
static const ter_str_id ter_t_swater_surf( "t_swater_surf" );
static const ter_str_id ter_t_tidepool( "t_tidepool" );
static const ter_str_id ter_t_tree_birch( "t_tree_birch" );
static const ter_str_id ter_t_tree_birch_harvested( "t_tree_birch_harvested" );
static const ter_str_id ter_t_tree_dead( "t_tree_dead" );
static const ter_str_id ter_t_tree_deadpine( "t_tree_deadpine" );
static const ter_str_id ter_t_tree_hickory( "t_tree_hickory" );
static const ter_str_id ter_t_tree_hickory_dead( "t_tree_hickory_dead" );
static const ter_str_id ter_t_tree_hickory_harvested( "t_tree_hickory_harvested" );
static const ter_str_id ter_t_tree_pine( "t_tree_pine" );
static const ter_str_id ter_t_tree_willow( "t_tree_willow" );
static const ter_str_id ter_t_trunk( "t_trunk" );
static const ter_str_id ter_t_water_dp( "t_water_dp" );
static const ter_str_id ter_t_water_moving_dp( "t_water_moving_dp" );
static const ter_str_id ter_t_water_moving_sh( "t_water_moving_sh" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );

static const vgroup_id VehicleGroup_crashed_helicopters( "crashed_helicopters" );

static const vproto_id vehicle_prototype_excavator( "excavator" );
static const vproto_id vehicle_prototype_road_roller( "road_roller" );

class item;
class npc_template;

namespace io
{

template<>
std::string enum_to_string<map_extra_method>( map_extra_method data )
{
    switch( data ) {
        // *INDENT-OFF*
        case map_extra_method::null: return "null";
        case map_extra_method::map_extra_function: return "map_extra_function";
        case map_extra_method::mapgen: return "mapgen";
        case map_extra_method::update_mapgen: return "update_mapgen";
        case map_extra_method::num_map_extra_methods: break;
        // *INDENT-ON*
    }
    cata_fatal( "Invalid map_extra_method" );
}

} // namespace io

namespace
{

generic_factory<map_extra> extras( "map extra" );

} // namespace

/** @relates string_id */
template<>
const map_extra &string_id<map_extra>::obj() const
{
    return extras.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<map_extra>::is_valid() const
{
    return extras.is_valid( *this );
}

namespace MapExtras
{

const generic_factory<map_extra> &mapExtraFactory()
{
    return extras;
}

void clear()
{
    extras.reset();
}

static bool mx_null( map &, const tripoint_abs_sm & )
{
    debugmsg( "Tried to generate null map extra." );

    return false;
}

static void dead_vegetation_parser( map &m, const tripoint_bub_ms &loc )
{
    // furniture plants die to withered plants
    const furn_t &fid = m.furn( loc ).obj();
    if( fid.has_flag( ter_furn_flag::TFLAG_PLANT ) || fid.has_flag( ter_furn_flag::TFLAG_FLOWER ) ||
        fid.has_flag( ter_furn_flag::TFLAG_ORGANIC ) ) {
        m.i_clear( loc );
        m.furn_set( loc, furn_str_id::NULL_ID() );
        m.spawn_item( loc, itype_withered );
    }
    // terrain specific conversions
    const ter_id &tid = m.ter( loc );
    static const std::map<ter_id, ter_str_id> dies_into {{
            {ter_t_grass, ter_t_grass_dead},
            {ter_t_grass_long, ter_t_grass_dead},
            {ter_t_grass_tall, ter_t_grass_dead},
            {ter_t_moss, ter_t_grass_dead},
            {ter_t_tree_pine, ter_t_tree_deadpine},
            {ter_t_tree_birch, ter_t_tree_birch_harvested},
            {ter_t_tree_willow, ter_t_tree_dead},
            {ter_t_tree_hickory, ter_t_tree_hickory_dead},
            {ter_t_tree_hickory_harvested, ter_t_tree_hickory_dead},
            {ter_t_grass_golf, ter_t_grass_dead},
            {ter_t_grass_white, ter_t_grass_dead},
        }};

    const auto iter = dies_into.find( tid );
    if( iter != dies_into.end() ) {
        m.ter_set( loc, iter->second );
    }
    // non-specific small vegetation falls into sticks, large dies and randomly falls
    const ter_t &tr = tid.obj();
    if( tr.has_flag( ter_furn_flag::TFLAG_SHRUB ) ) {
        m.ter_set( loc, ter_t_dirt );
        if( one_in( 2 ) ) {
            m.spawn_item( loc, itype_stick );
        }
    } else if( tr.has_flag( ter_furn_flag::TFLAG_TREE ) ) {
        if( one_in( 4 ) ) {
            m.ter_set( loc, ter_t_trunk );
        } else if( one_in( 4 ) ) {
            m.ter_set( loc, ter_t_stump );
        } else {
            m.ter_set( loc, ter_t_tree_dead );
        }
    } else if( tr.has_flag( ter_furn_flag::TFLAG_YOUNG ) ) {
        m.ter_set( loc, ter_t_dirt );
        if( one_in( 2 ) ) {
            m.spawn_item( loc, itype_stick_long );
        }
    }
}

static void delete_items_at_mount( vehicle &veh, const point_rel_ms &pt )
{
    for( const int idx_cargo : veh.parts_at_relative( pt, /* use_cache = */ true ) ) {
        vehicle_part &vp_cargo = veh.part( idx_cargo );
        veh.get_items( vp_cargo ).clear();
    }
}

static bool mx_helicopter( map &m, const tripoint_abs_sm &abs_sub )
{
    point_bub_ms c = rng_map_point<point_bub_ms>( 6 );

    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            const tripoint_bub_ms pos( x,  y, abs_sub.z() );
            if( m.veh_at( pos ) &&
                m.ter( pos )->has_flag( ter_furn_flag::TFLAG_DIGGABLE ) ) {
                m.ter_set( pos, ter_t_dirtmound );
            } else {
                if( x >= c.x() - dice( 1, 5 ) && x <= c.x() + dice( 1, 5 ) && y >= c.y() - dice( 1, 5 ) &&
                    y <= c.y() + dice( 1, 5 ) ) {
                    if( one_in( 7 ) &&
                        m.ter( pos )->has_flag( ter_furn_flag::TFLAG_DIGGABLE ) ) {
                        m.ter_set( pos, ter_t_dirtmound );
                    }
                }
                if( x >= c.x() - dice( 1, 6 ) && x <= c.x() + dice( 1, 6 ) && y >= c.y() - dice( 1, 6 ) &&
                    y <= c.y() + dice( 1, 6 ) ) {
                    if( !one_in( 5 ) ) {
                        m.make_rubble( pos, furn_f_wreckage, true );
                        if( m.ter( pos )->has_flag( ter_furn_flag::TFLAG_DIGGABLE ) ) {
                            m.ter_set( pos, ter_t_dirtmound );
                        }
                    } else if( m.is_bashable( pos ) ) {
                        m.destroy( pos, true );
                        if( m.ter( pos )->has_flag( ter_furn_flag::TFLAG_DIGGABLE ) ) {
                            m.ter_set( pos, ter_t_dirtmound );
                        }
                    }

                } else if( one_in( 4 + ( std::abs( x - c.x() ) + std::abs( y -
                                         c.y() ) ) ) ) { // 1 in 10 chance of being wreckage anyway
                    m.make_rubble( pos, furn_f_wreckage, true );
                    if( !one_in( 3 ) ) {
                        if( m.ter( pos )->has_flag( ter_furn_flag::TFLAG_DIGGABLE ) ) {
                            m.ter_set( pos, ter_t_dirtmound );
                        }
                    }
                }
            }
        }
    }

    units::angle dir1 = random_direction();

    vproto_id crashed_hull = VehicleGroup_crashed_helicopters->pick();

    tripoint_bub_ms wreckage_pos;
    {
        // veh should fall out of scope, don't actually use it, create the vehicle so
        // we can rotate it and calculate its bounding box, but don't place it on the map.
        vehicle veh( crashed_hull );
        veh.turn( dir1 );
        // Find where the center of the vehicle is and adjust the spawn position to get it centered in the
        // debris area.
        const bounding_box bbox = veh.get_bounding_box();

        point_rel_ms center_offset = {( bbox.p1.x() + bbox.p2.x() ) / 2, ( bbox.p1.y() + bbox.p2.y() ) / 2};
        // Clamp x1 & y1 such that no parts of the vehicle extend over the border of the submap.

        wreckage_pos = { clamp( c.x() - center_offset.x(), std::abs( bbox.p1.x() ), SEEX * 2 - 1 - std::abs( bbox.p2.x() ) ),
                         clamp( c.y() - center_offset.y(), std::abs( bbox.p1.y() ), SEEY * 2 - 1 - std::abs( bbox.p2.y() ) ),
                         abs_sub.z()
                       };
    }

    vehicle *wreckage = m.add_vehicle( crashed_hull, wreckage_pos, dir1, rng( 1, 33 ), 1 );

    const auto controls_at = [&m]( vehicle * wreckage, const tripoint_bub_ms & pos ) {
        return !wreckage->get_parts_at( &m, pos, "CONTROLS", part_status_flag::any ).empty() ||
               !wreckage->get_parts_at( &m, pos, "CTRL_ELECTRONIC", part_status_flag::any ).empty();
    };

    if( wreckage != nullptr ) {
        const int clowncar_factor = dice( 1, 8 );

        switch( clowncar_factor ) {
            case 1:
            case 2:
            case 3:
                // Full clown car
                for( const vpart_reference &vp : wreckage->get_any_parts( VPFLAG_SEATBELT ) ) {
                    const tripoint_bub_ms pos = vp.pos_bub( m );
                    // Spawn pilots in seats with controls.CTRL_ELECTRONIC
                    if( controls_at( wreckage, pos ) ) {
                        m.place_spawns( GROUP_MIL_PILOT, 1, pos.xy(), pos.xy(), pos.z(), 1, true );
                    } else {
                        m.place_spawns( GROUP_MIL_PASSENGER, 1, pos.xy(), pos.xy(), pos.z(), 1, true );
                    }
                    delete_items_at_mount( *wreckage, vp.mount_pos() ); // delete corpse items
                }
                break;
            case 4:
            case 5:
                // 2/3rds clown car
                for( const vpart_reference &vp : wreckage->get_any_parts( VPFLAG_SEATBELT ) ) {
                    const tripoint_bub_ms pos = vp.pos_bub( m );
                    // Spawn pilots in seats with controls.
                    if( controls_at( wreckage, pos ) ) {
                        m.place_spawns( GROUP_MIL_PILOT, 1, pos.xy(), pos.xy(), pos.z(), 1, true );
                    } else {
                        m.place_spawns( GROUP_MIL_WEAK, 2, pos.xy(), pos.xy(), pos.z(), 1, true );
                    }
                    delete_items_at_mount( *wreckage, vp.mount_pos() ); // delete corpse items
                }
                break;
            case 6:
                // Just pilots
                for( const vpart_reference &vp : wreckage->get_any_parts( VPFLAG_CONTROLS ) ) {
                    const tripoint_bub_ms pos = vp.pos_bub( m );
                    m.place_spawns( GROUP_MIL_PILOT, 1, pos.xy(), pos.xy(), pos.z(), 1, true );
                    delete_items_at_mount( *wreckage, vp.mount_pos() ); // delete corpse items
                }
                break;
            case 7:
            // Empty clown car
            case 8:
            default:
                break;
        }
        if( !one_in( 4 ) ) {
            wreckage->smash( m, 0.8f, 1.2f, 1.0f, { dice( 1, 8 ) - 5, dice( 1, 8 ) - 5 }, 6 + dice( 1,
                             10 ) );
        } else {
            wreckage->smash( m, 0.1f, 0.9f, 1.0f, { dice( 1, 8 ) - 5, dice( 1, 8 ) - 5 }, 6 + dice( 1,
                             10 ) );
        }
    }

    return true;
}

static void place_fumarole( map &m, const point_bub_ms &p1, const point_bub_ms &p2,
                            std::set<point_bub_ms> &ignited )
{
    // Tracks points nearby for ignition after the lava is placed
    //std::set<point_bub_ms> ignited;

    std::vector<point_bub_ms> fumarole = line_to( p1, p2, 0 );
    for( point_bub_ms &i : fumarole ) {
        m.ter_set( i, ter_t_lava );

        // Add all adjacent tiles (even on diagonals) for possible ignition
        // Since they're being added to a set, duplicates won't occur
        ignited.insert( ( i + point::north_west ) );
        ignited.insert( ( i + point::north ) );
        ignited.insert( ( i + point::north_east ) );
        ignited.insert( ( i + point::west ) );
        ignited.insert( ( i + point::east ) );
        ignited.insert( ( i + point::south_west ) );
        ignited.insert( ( i + point::south ) );
        ignited.insert( ( i + point::south_east ) );

        if( one_in( 6 ) ) {
            m.spawn_item( i + point::north_west, itype_chunk_sulfur );
        }
    }

}

static bool mx_portal_in( map &m, const tripoint_abs_sm &abs_sub )
{
    static constexpr int omt_size = SEEX * 2;
    // minimum 9 tiles from the edge because ARTPROP_FRACTAL calls
    // create_anomaly at an offset of 4, and create_anomaly generates a
    // furniture circle around that of radius 5.
    static constexpr int min_coord = 9;
    static constexpr int max_coord = omt_size - 1 - min_coord;
    static_assert( min_coord < max_coord, "no space for randomness" );
    const tripoint_bub_ms portal_location{
        rng( min_coord, max_coord ), rng( min_coord, max_coord ), abs_sub.z()};
    const point_bub_ms p( portal_location.xy() );

    switch( rng( 1, 6 ) ) {
        //Mycus spreading through the portal
        case 1: {
            m.add_field( portal_location, fd_fatigue, 3 );
            fungal_effects fe;
            for( const tripoint_bub_ms &loc : m.points_in_radius( portal_location, 5 ) ) {
                if( one_in( 3 ) ) {
                    fe.marlossify( loc );
                }
            }
            //50% chance to spawn pouf-maker
            m.place_spawns( GROUP_FUNGI_FUNGALOID, 2, p + point::north_west, p + point::south_east,
                            portal_location.z(), 1, true );
            break;
        }
        //Netherworld monsters spawning around the portal
        case 2: {
            m.add_field( portal_location, fd_fatigue, 3 );
            for( const tripoint_bub_ms &loc : m.points_in_radius( portal_location, 5 ) ) {
                m.place_spawns( GROUP_NETHER_PORTAL, 15, loc.xy(), loc.xy(), loc.z(), 1, true );
            }
            break;
        }
        //Several cracks in the ground originating from the portal
        case 3: {
            m.add_field( portal_location, fd_fatigue, 3 );
            for( int i = 0; i < rng( 1, 10 ); i++ ) {
                tripoint_bub_ms end_location = rng_map_point<tripoint_bub_ms>( 0, abs_sub.z() );
                std::vector<tripoint_bub_ms> failure = line_to( portal_location, end_location );
                for( tripoint_bub_ms &i : failure ) {
                    m.ter_set( tripoint_bub_ms{ i.xy(), abs_sub.z()}, ter_t_pit );
                }
            }
            break;
        }
        //Radiation from the portal killed the vegetation
        case 4: {
            m.add_field( portal_location, fd_fatigue, 3 );
            const int rad = 5;
            for( int i = p.x() - rad; i <= p.x() + rad; i++ ) {
                for( int j = p.y() - rad; j <= p.y() + rad; j++ ) {
                    if( trig_dist( p.raw(), point( i, j ) ) + rng( 0, 3 ) <= rad ) {
                        const tripoint_bub_ms loc( i, j, abs_sub.z() );
                        dead_vegetation_parser( m, loc );
                        m.adjust_radiation( loc.xy(), rng( 20, 40 ) );
                    }
                }
            }
            break;
        }
        //Lava seams originating from the portal
        case 5: {
            if( abs_sub.z() <= 0 ) {
                point_bub_ms p1( rng( 1,    SEEX     - 3 ), rng( 1,    SEEY     - 3 ) );
                point_bub_ms p2( rng( SEEX, SEEX * 2 - 3 ), rng( SEEY, SEEY * 2 - 3 ) );
                // Pick a random cardinal direction to also spawn lava in
                // This will make the lava a single connected line, not just on diagonals
                static const std::array<direction, 4> possibilities = { { direction::EAST, direction::WEST, direction::NORTH, direction::SOUTH } };
                const direction extra_lava_dir = random_entry( possibilities );
                point_rel_ms extra;
                switch( extra_lava_dir ) {
                    case direction::NORTH:
                        extra.y() = -1;
                        break;
                    case direction::EAST:
                        extra.x() = 1;
                        break;
                    case direction::SOUTH:
                        extra.y() = 1;
                        break;
                    case direction::WEST:
                        extra.x() = -1;
                        break;
                    default:
                        break;
                }

                const tripoint_bub_ms portal_location = { p1 + tripoint_rel_ms( extra, abs_sub.z() ) };
                m.add_field( portal_location, fd_fatigue, 3 );

                std::set<point_bub_ms> ignited;
                place_fumarole( m, p1, p2, ignited );
                place_fumarole( m, p1 + extra, p2 + extra,
                                ignited );

                for( const point_bub_ms &i : ignited ) {
                    // Don't need to do anything to tiles that already have lava on them
                    if( m.ter( i ) != ter_t_lava ) {
                        // Spawn an intense but short-lived fire
                        // Any furniture or buildings will catch fire, otherwise it will burn out quickly
                        m.add_field( tripoint_bub_ms( i, abs_sub.z() ), fd_fire, 15, 1_minutes );
                    }
                }
            }
            break;
        }
        //Anomaly caused by the portal and spawned an artifact
        case 6: {
            m.add_field( portal_location, fd_fatigue, 3 );
            artifact_natural_property prop =
                static_cast<artifact_natural_property>( rng( ARTPROP_NULL + 1, ARTPROP_MAX - 1 ) );
            m.create_anomaly( portal_location, prop );
            m.spawn_artifact( p + tripoint( rng( -1, 1 ), rng( -1, 1 ), abs_sub.z() ),
                              relic_procgen_data_alien_reality, 5, 1000, -2000, true );
            break;
        }
    }

    return true;
}

static bool mx_helper_clone_plant( map &m, const tripoint_abs_sm &abs_sub, ter_furn_flag tfflag )
{
    // From wikipedia - The main meaning of "grove" is a group of trees that grow close together,
    // generally without many bushes or other plants underneath.

    // This map extra helper finds the first appropriately flagged terrain in the area, and then
    // converts all trees, young trees, and shrubs in the area into that type.

    ter_id tree;
    bool found_tree = false;
    for( int i = 0; !found_tree && i < SEEX * 2; i++ ) {
        for( int j = 0; !found_tree && j < SEEY * 2; j++ ) {
            const tripoint_bub_ms location( i, j, abs_sub.z() );
            if( m.has_flag_ter( tfflag, location ) ) {
                tree = m.ter( location );
                found_tree = true;
            }
        }
    }

    if( !found_tree ) {
        return false;
    }

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            const tripoint_bub_ms location( i, j, abs_sub.z() );
            if( m.has_flag_ter( ter_furn_flag::TFLAG_SHRUB, location ) ||
                m.has_flag_ter( ter_furn_flag::TFLAG_TREE, location ) ||
                m.has_flag_ter( ter_furn_flag::TFLAG_YOUNG, location ) ) {
                m.ter_set( location, tree );
            }
        }
    }

    return true;
}

static bool mx_shrubbery( map &m, const tripoint_abs_sm &abs_sub )
{
    return mx_helper_clone_plant( m, abs_sub, ter_furn_flag::TFLAG_SHRUB );
}

static bool mx_grove( map &m, const tripoint_abs_sm &abs_sub )
{
    return mx_helper_clone_plant( m, abs_sub, ter_furn_flag::TFLAG_TREE );
}

static bool mx_pond( map &m, const tripoint_abs_sm &abs_sub )
{
    // This map extra creates small ponds using a simple cellular automaton.

    constexpr int width = SEEX * 2;
    constexpr int height = SEEY * 2;

    // Generate the cells for our lake.
    std::vector<std::vector<int>> current = CellularAutomata::generate_cellular_automaton( width,
                                            height, 55, 5, 4, 3 );

    // Loop through and turn every live cell into water.
    // Do a roll for our three possible lake types:
    // - all deep water
    // - all shallow water
    // - shallow water on the shore, deep water in the middle
    const int lake_type = rng( 1, 3 );
    for( int i = 0; i < width; i++ ) {
        for( int j = 0; j < height; j++ ) {
            if( current[i][j] == 1 ) {
                const tripoint_bub_ms location( i, j, abs_sub.z() );
                m.furn_set( location, furn_str_id::NULL_ID() );

                switch( lake_type ) {
                    case 1:
                        m.ter_set( location, ter_t_water_sh );
                        break;
                    case 2:
                        m.ter_set( location, ter_t_water_dp );
                        break;
                    case 3:
                        const int neighbors = CellularAutomata::neighbor_count( current, width, height, point( i, j ) );
                        if( neighbors == 8 ) {
                            m.ter_set( location, ter_t_water_dp );
                        } else {
                            m.ter_set( location, ter_t_water_sh );
                        }
                        break;
                }
            }
        }
    }

    m.place_spawns( GROUP_FISH, 1, point_bub_ms::zero, point_bub_ms( width, height ),
                    abs_sub.z(),
                    0.15f );
    return true;
}

static bool mx_reed( map &m, const tripoint_abs_sm &abs_sub )
{
    // This map extra is for populating river banks, lake shores, etc. with
    // water vegetation

    // intensity consistent for whole overmap terrain bit
    // so vegetation is placed from one_in(1) = full overgrowth
    // to one_in(4) = 1/4 of terrain;
    int intensity = rng( 1, 4 );

    const auto near_water = [ &m ]( tripoint_bub_ms loc ) {

        for( const tripoint_bub_ms &p : m.points_in_radius( loc, 1 ) ) {
            if( p == loc ) {
                continue;
            }
            const ter_id &t = m.ter( p );
            if( t == ter_t_water_moving_sh || t == ter_t_water_sh ||
                t == ter_t_water_moving_dp || t == ter_t_water_dp ) {
                return true;
            }
        }
        return false;
    };

    weighted_int_list<furn_id> vegetation;
    vegetation.add( furn_f_cattails, 15 );
    vegetation.add( furn_f_lotus, 5 );
    vegetation.add( furn_id( "f_purple_loosestrife" ), 1 );
    vegetation.add( furn_f_lilypad, 1 );
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            const tripoint_bub_ms loc( i, j, abs_sub.z() );
            const ter_id &ter_loc = m.ter( loc );
            if( ( ter_loc == ter_t_water_sh || ter_loc == ter_t_water_moving_sh ) &&
                one_in( intensity ) ) {
                m.furn_set( loc, vegetation.pick()->id() );
            }
            // tall grass imitates reed
            if( ( ter_loc == ter_t_dirt || ter_loc == ter_t_grass ) &&
                one_in( near_water( loc ) ? intensity : 7 ) ) {
                m.ter_set( loc, ter_t_grass_tall );
            }
        }
    }

    return true;
}

static bool mx_roadworks( map &m, const tripoint_abs_sm &abs_sub )
{
    // This map extra creates road works on NS & EW roads, including barricades (as barrier poles),
    // holes in the road, scattered soil, chance for heavy utility vehicles and some working
    // equipment in a box
    // (curved roads & intersections excluded, perhaps TODO)

    const tripoint_abs_omt abs_omt( coords::project_to<coords::omt>( abs_sub ) );
    const oter_id &north = overmap_buffer.ter( abs_omt + point::north );
    const oter_id &south = overmap_buffer.ter( abs_omt + point::south );
    const oter_id &west = overmap_buffer.ter( abs_omt + point::west );
    const oter_id &east = overmap_buffer.ter( abs_omt + point::east );

    const bool road_at_north = north->get_type_id() == oter_type_road;
    const bool road_at_south = south->get_type_id() == oter_type_road;
    const bool road_at_west = west->get_type_id() == oter_type_road;
    const bool road_at_east = east->get_type_id() == oter_type_road;

    // defect types
    weighted_int_list<ter_id> road_defects;
    road_defects.add( ter_t_pit_shallow, 15 );
    road_defects.add( ter_t_dirt, 15 );
    road_defects.add( ter_t_dirtmound, 15 );
    road_defects.add( ter_t_pavement, 55 );
    const weighted_int_list<ter_id> defects = road_defects;

    // location holders
    point_bub_ms defects_from; // road defects square start
    point_bub_ms defects_to; // road defects square end
    point_bub_ms defects_centered; //  road defects centered
    tripoint_bub_ms veh; // vehicle
    point_bub_ms equipment; // equipment

    bool straight = true;
    int turns = -1;
    // determine placement of effects
    if( road_at_north && road_at_south && !road_at_east && !road_at_west ) {
        if( one_in( 2 ) ) { // west side of the NS road
            turns = 0;
        } else { // east side of the NS road
            turns = 2;
        }
    } else if( road_at_west && road_at_east && !road_at_north && !road_at_south ) {
        if( one_in( 2 ) ) { // north side of the EW road
            turns = 1;
        } else { // south side of the EW road
            turns = 3;
        }
    } else if( road_at_north && road_at_east && !road_at_west && !road_at_south ) {
        straight = false;
        turns = 0;
    } else if( road_at_south && road_at_west && !road_at_east && !road_at_north ) {
        straight = false;
        turns = 2;
    } else if( road_at_north && road_at_west && !road_at_east && !road_at_south ) {
        straight = false;
        turns = 3;
    } else if( road_at_south && road_at_east && !road_at_west && !road_at_north ) {
        straight = false;
        turns = 1;
    } else {
        return false; // crossroads and strange roads - no generation, bail out
    }

    if( straight ) {
        // road barricade
        line_furn( &m, furn_f_barricade_road,
                   point_bub_ms( 4, 0 ).rotate_in_map( turns ), point_bub_ms( 11, 7 ).rotate_in_map( turns ),
                   abs_sub.z() );
        line_furn( &m, furn_f_barricade_road,
                   point_bub_ms( 11, 8 ).rotate_in_map( turns ), point_bub_ms( 11, 15 ).rotate_in_map( turns ),
                   abs_sub.z() );
        line_furn( &m, furn_f_barricade_road,
                   point_bub_ms( 11, 16 ).rotate_in_map( turns ), point_bub_ms( 4, 23 ).rotate_in_map( turns ),
                   abs_sub.z() );
        // road defects
        defects_from = point_bub_ms( 9, 7 ).rotate_in_map( turns );
        defects_to = point_bub_ms( 4, 16 ).rotate_in_map( turns );
        defects_centered = point_bub_ms( rng( 4, 7 ), rng( 10, 16 ) ).rotate_in_map( turns );
        // vehicle
        veh = tripoint_bub_ms( rng( 4, 6 ), rng( 8, 10 ), abs_sub.z() ).rotate_in_map( turns );
        // equipment
        if( one_in( 2 ) ) {
            equipment = point_bub_ms( rng( 0, 4 ), rng( 1, 2 ) ).rotate_in_map( turns );
        } else {
            equipment = point_bub_ms( rng( 0, 4 ), rng( 21, 22 ) ).rotate_in_map( turns );
        }
    } else {
        // road barricade
        line_furn( &m, furn_f_barricade_road,
                   // NOLINTNEXTLINE(cata-use-named-point-constants)
                   point_bub_ms( 1, 0 ).rotate_in_map( turns ), point_bub_ms( 11, 0 ).rotate_in_map( turns ),
                   abs_sub.z() );
        line_furn( &m, furn_f_barricade_road,
                   point_bub_ms( 12, 0 ).rotate_in_map( turns ), point_bub_ms( 23, 10 ).rotate_in_map( turns ),
                   abs_sub.z() );
        line_furn( &m, furn_f_barricade_road,
                   point_bub_ms( 23, 22 ).rotate_in_map( turns ), point_bub_ms( 23, 11 ).rotate_in_map( turns ),
                   abs_sub.z() );
        // road defects
        switch( rng( 1, 3 ) ) {
            case 1:
                defects_from = point_bub_ms( 9, 8 ).rotate_in_map( turns );
                defects_to = point_bub_ms( 14, 3 ).rotate_in_map( turns );
                break;
            case 2:
                defects_from = point_bub_ms( 12, 11 ).rotate_in_map( turns );
                defects_to = point_bub_ms( 17, 6 ).rotate_in_map( turns );
                break;
            case 3:
                defects_from = point_bub_ms( 16, 15 ).rotate_in_map( turns );
                defects_to = point_bub_ms( 21, 10 ).rotate_in_map( turns );
                break;
        }
        defects_centered = point_bub_ms( rng( 8, 14 ), rng( 8, 14 ) ).rotate_in_map( turns );
        // vehicle
        veh = tripoint_bub_ms( rng( 7, 15 ), rng( 7, 15 ), abs_sub.z() ).rotate_in_map( turns );
        // equipment
        if( one_in( 2 ) ) {
            equipment = point_bub_ms( rng( 0, 1 ), rng( 2, 23 ) ).rotate_in_map( turns );
        } else {
            equipment = point_bub_ms( rng( 0, 22 ), rng( 22, 23 ) ).rotate_in_map( turns );
        }
    }
    // road defects generator
    switch( rng( 1, 5 ) ) {
        case 1:
            square( &m, defects, defects_from,
                    defects_to );
            break;
        case 2:
            rough_circle( &m, ter_t_pit_shallow, defects_centered, rng( 2, 4 ) );
            break;
        case 3:
            circle( &m, ter_t_pit_shallow, defects_centered, rng( 2, 4 ) );
            break;
        case 4:
            rough_circle( &m, ter_t_dirtmound, defects_centered, rng( 2, 4 ) );
            break;
        case 5:
            circle( &m, ter_t_dirtmound, defects_centered, rng( 2, 4 ) );
            break;
    }
    // soil generator
    for( int i = 1; i <= 10; i++ ) {
        m.spawn_item( point_bub_ms( rng( defects_from.x(), defects_to.x() ),
                                    rng( defects_from.y(), defects_to.y() ) ), itype_material_soil );
    }
    // vehicle placer
    switch( rng( 1, 6 ) ) {
        case 1:
            m.add_vehicle( vehicle_prototype_road_roller, veh, random_direction() );
            break;
        case 2:
            m.add_vehicle( vehicle_prototype_excavator, veh, random_direction() );
            break;
        case 3:
        case 4:
        case 5:
        case 6:
            break;
            // 3-6 empty to reduce chance of vehicles on site
    }
    // equipment placer
    if( one_in( 3 ) ) {
        m.furn_set( equipment, furn_f_crate_c );
        m.place_items( Item_spawn_data_mine_equipment, 100, tripoint_bub_ms( equipment, abs_sub.z() ),
                       tripoint_bub_ms( equipment, abs_sub.z() ), true, calendar::start_of_cataclysm, 100 );
    }

    return true;
}

static bool mx_casings( map &m, const tripoint_abs_sm &abs_sub )
{
    const std::vector<item> items = item_group::items_from( Item_spawn_data_ammo_casings,
                                    calendar::turn );

    // Spawn a pile of casings around location
    const auto spawn_casings_pile = [&m]( const tripoint_bub_ms & location,
    const std::vector<item> &items ) {
        for( const tripoint_bub_ms &loc : m.points_in_radius( location, rng( 1, 2 ) ) ) {
            if( one_in( 2 ) ) {
                m.spawn_items( loc, items );
            }
        }
    };

    //Spawn blood and bloody rag and sometimes trail of blood
    const auto spawn_blood = [&m]( const tripoint_bub_ms & location, const int radius, bool trail ) {
        if( one_in( 2 ) ) {
            m.add_field( location, fd_blood, rng( 1, 3 ) );
            if( one_in( 2 ) ) {
                const tripoint_bub_ms bloody_rag_loc = random_entry( m.points_in_radius( location, radius ) );
                m.spawn_item( bloody_rag_loc, itype_sheet_cotton, 1, 0, calendar::start_of_cataclysm, 0, { json_flag_FILTHY } );
            }
            if( trail && one_in( 2 ) ) {
                m.add_splatter_trail( fd_blood, location,
                                      random_entry( m.points_in_radius( location, rng( 1, 4 ) ) ) );
            }
        }
    };

    // Spawn random trash
    const int num = rng( 1, 3 );
    for( int i = 0; i < num; i++ ) {
        const std::vector<item> trash =
            item_group::items_from( Item_spawn_data_map_extra_casings, calendar::turn );
        m.spawn_items( rng_map_point<tripoint_bub_ms>( 1, abs_sub.z() ), trash );
    }

    const tripoint_bub_ms random_loc = rng_map_point<tripoint_bub_ms>( 1, abs_sub.z() );
    switch( rng( 1, 4 ) ) {
        //Pile of random casings in random place
        case 1: {
            spawn_casings_pile( random_loc, items );
            spawn_blood( random_loc, 3, true );
            break;
        }
        //Entire battlefield filled with casings
        case 2: {
            //Spawn casings
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    if( one_in( 20 ) ) {
                        m.spawn_items( { i, j, abs_sub.z()}, items );
                    }
                }
            }
            spawn_blood( { SEEX, SEEY, abs_sub.z() }, rng( 1, 10 ), false );
            break;
        }
        //Person moved and fired in some direction
        case 3: {
            //Spawn casings and blood trail along the direction of movement
            const tripoint_bub_ms to = rng_map_point<tripoint_bub_ms>( 1, abs_sub.z() );
            std::vector<tripoint_bub_ms> casings = line_to( random_loc, to );
            for( const tripoint_bub_ms &i : casings ) {
                if( one_in( 2 ) ) {
                    m.spawn_items( { i.xy(), abs_sub.z()}, items );
                    if( one_in( 2 ) ) {
                        m.add_field( { i.xy(), abs_sub.z()}, fd_blood, rng( 1, 3 ) );
                    }
                }
            }
            spawn_blood( to, 3, false );
            break;
        }
        //Two persons shot and created two piles of casings
        case 4: {
            const tripoint_bub_ms second_loc = rng_map_point<tripoint_bub_ms>( 1, abs_sub.z() );
            const std::vector<item> second_items =
                item_group::items_from( Item_spawn_data_ammo_casings, calendar::turn );
            spawn_casings_pile( random_loc, items );
            spawn_casings_pile( second_loc, second_items );
            spawn_blood( random_loc, 3, true );
            spawn_blood( second_loc, 3, true );
            break;
        }
    }

    return true;
}

static bool mx_looters( map &m, const tripoint_abs_sm &abs_sub )
{
    const tripoint_bub_ms center = rng_map_point<tripoint_bub_ms>( 4, abs_sub.z() );
    //25% chance to spawn a corpse with some blood around it
    if( one_in( 4 ) && m.passable_through( center ) ) {
        m.add_corpse( center );
        const tripoint_range<tripoint_bub_ms> points = m.points_in_radius( center, 1 );
        const int num = rng( 1, 3 );
        for( int i = 0; i < num; i++ ) {
            m.add_field( random_entry( points ), fd_blood, rng( 1, 3 ) );
        }
    }

    //Spawn up to 5 hostile bandits with equal chance to be ranged or melee type
    const int num_looters = rng( 1, 5 );
    const tripoint_range<tripoint_bub_ms> looter_points = m.points_in_radius( center, rng( 1, 4 ) );
    for( int i = 0; i < num_looters; i++ ) {
        const std::optional<tripoint_bub_ms> pos_ =
        random_point( looter_points, [&]( const tripoint_bub_ms & p ) {
            return m.passable_through( p );
        } );
        if( pos_ ) {
            m.place_npc( pos_->xy(), string_id<npc_template>( one_in( 2 ) ? "thug" : "bandit" ) );
        }
    }

    return true;
}

static bool mx_corpses( map &m, const tripoint_abs_sm &abs_sub )
{
    const int num_corpses = rng( 1, 5 );
    //Spawn up to 5 human corpses in random places
    for( int i = 0; i < num_corpses; i++ ) {
        const tripoint_bub_ms corpse_location = rng_map_point<tripoint_bub_ms>( 1, abs_sub.z() );
        if( m.passable_through( corpse_location ) ) {
            m.add_field( corpse_location, fd_blood, rng( 1, 3 ) );
            m.put_items_from_loc( Item_spawn_data_everyday_corpse, corpse_location );
            //50% chance to spawn blood in every tile around every corpse in 1-tile radius
            for( const tripoint_bub_ms &loc : m.points_in_radius( corpse_location, 1 ) ) {
                if( one_in( 2 ) ) {
                    m.add_field( loc, fd_blood, rng( 1, 3 ) );
                }
            }
        }
    }
    //10% chance to spawn a flock of stray dogs feeding on human flesh
    if( one_in( 10 ) && num_corpses <= 4 ) {
        const tripoint_bub_ms corpse_location = rng_map_point<tripoint_bub_ms>( 1, abs_sub.z() );
        const std::vector<item> gibs =
            item_group::items_from( Item_spawn_data_remains_human_generic,
                                    calendar::start_of_cataclysm );
        m.spawn_items( corpse_location, gibs );
        m.add_field( corpse_location, fd_gibs_flesh, rng( 1, 3 ) );
        //50% chance to spawn gibs and dogs in every tile around what's left of human corpse in 1-tile radius
        for( const tripoint_bub_ms &loc : m.points_in_radius( corpse_location, 1 ) ) {
            if( one_in( 2 ) ) {
                m.add_field( { loc.xy(), abs_sub.z()}, fd_gibs_flesh, rng( 1, 3 ) );
                m.place_spawns( GROUP_STRAY_DOGS, 1, loc.xy(), loc.xy(), loc.z(), 1, true );
            }
        }
    }

    return true;
}

static bool mx_fungal_zone( map &m, const tripoint_abs_sm &abs_sub )
{
    // Find suitable location for fungal spire to spawn (grass, dirt etc)
    const tripoint_bub_ms omt_center = { SEEX, SEEY, abs_sub.z()};
    std::vector<tripoint_bub_ms> suitable_locations;
    for( const tripoint_bub_ms &loc : m.points_in_radius( omt_center, 10 ) ) {
        if( m.has_flag_ter( ter_furn_flag::TFLAG_DIGGABLE, loc ) ) {
            suitable_locations.push_back( loc );
        }
    }

    // If there's no suitable location found, bail out
    if( suitable_locations.empty() ) {
        return false;
    }

    const tripoint_bub_ms suitable_location = random_entry( suitable_locations, omt_center );
    m.add_spawn( mon_fungaloid_queen, 1, suitable_location );
    m.place_spawns( GROUP_FUNGI_FUNGALOID, 1,
                    suitable_location.xy() + point::north_west,
                    suitable_location.xy() + point::south_east,
                    suitable_location.z(),
                    3, true );
    return true;
}

static bool mx_sandy_beach( map &m, const tripoint_abs_sm &abs_sub )
{
    weighted_int_list<furn_id> detritus;
    detritus.add( furn_f_beach_log, 10 );
    detritus.add( furn_f_beach_seaweed, 25 );
    detritus.add( furn_f_boulder_small, 20 );
    detritus.add( furn_f_boulder_medium, 10 );
    detritus.add( furn_f_boulder_large, 3 );

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            const tripoint_bub_ms loc( i, j, abs_sub.z() );
            const ter_id &ter_loc = m.ter( loc );
            if( ter_loc == ter_t_sand && one_in( 20 ) ) {
                !one_in( 10 ) ? m.furn_set( loc, detritus.pick()->id() ) : m.ter_set( loc, ter_t_tidepool );
            }
            if( ter_loc == ter_t_swater_surf && one_in( 10 ) ) {
                m.ter_set( loc, ter_t_coast_rock_surf );
            }
        }
    }

    return true;
}

static FunctionMap builtin_functions = {
    { map_extra_mx_null, mx_null },
    { map_extra_mx_roadworks, mx_roadworks },
    { map_extra_mx_helicopter, mx_helicopter },
    { map_extra_mx_portal_in, mx_portal_in },
    { map_extra_mx_grove, mx_grove },
    { map_extra_mx_shrubbery, mx_shrubbery },
    { map_extra_mx_pond, mx_pond },
    { map_extra_mx_casings, mx_casings },
    { map_extra_mx_looters, mx_looters },
    { map_extra_mx_corpses, mx_corpses },
    { map_extra_mx_reed, mx_reed },
    { map_extra_mx_fungal_zone, mx_fungal_zone },
    { map_extra_mx_sandy_beach, mx_sandy_beach },
};

map_extra_pointer get_function( const map_extra_id &name )
{
    const auto iter = builtin_functions.find( name );
    if( iter == builtin_functions.end() ) {
        debugmsg( "no built-in map extra function with id %s", name.str() );
        return nullptr;
    }
    return iter->second;
}

static std::vector<map_extra_id> all_function_names;
std::vector<map_extra_id> get_all_function_names()
{
    return all_function_names;
}

void apply_function( const map_extra_id &id, map &m, const tripoint_abs_sm &abs_sub )
{
    bool applied_successfully = false;

    const map_extra &extra = id.obj();
    switch( extra.generator_method ) {
        case map_extra_method::map_extra_function: {
            const map_extra_pointer mx_func = get_function( map_extra_id( extra.generator_id ) );
            if( mx_func != nullptr ) {
                applied_successfully = mx_func( m, abs_sub );
            }
            break;
        }
        case map_extra_method::mapgen: {
            mapgendata dat( project_to<coords::omt>( abs_sub ), m, 0.0f, calendar::turn,
                            nullptr );
            applied_successfully = run_mapgen_func( extra.generator_id, dat );
            break;
        }
        case map_extra_method::update_mapgen: {
            mapgendata dat( project_to<coords::omt>( abs_sub ), m, 0.0f,
                            calendar::start_of_cataclysm, nullptr );
            applied_successfully =
                run_mapgen_update_func( update_mapgen_id( extra.generator_id ), dat ).success();
            break;
        }
        case map_extra_method::null:
        default:
            break;
    }

    if( !applied_successfully ) {
        return;
    }

    overmap_buffer.add_extra( project_to<coords::omt>( abs_sub ), id );
}

void apply_function( const map_extra_id &id, tinymap &m, const tripoint_abs_omt &abs_omt )
{
    bool applied_successfully = false;

    const map_extra &extra = id.obj();
    switch( extra.generator_method ) {
        case map_extra_method::map_extra_function: {
            const map_extra_pointer mx_func = get_function( map_extra_id( extra.generator_id ) );
            if( mx_func != nullptr ) {
                applied_successfully = mx_func( *m.cast_to_map(), project_to<coords::sm>( abs_omt ) );
            }
            break;
        }
        case map_extra_method::mapgen: {
            mapgendata dat( abs_omt, *m.cast_to_map(), 0.0f, calendar::turn,
                            nullptr );
            applied_successfully = run_mapgen_func( extra.generator_id, dat );
            break;
        }
        case map_extra_method::update_mapgen: {
            mapgendata dat( abs_omt, *m.cast_to_map(), 0.0f,
                            calendar::start_of_cataclysm, nullptr );
            applied_successfully =
                run_mapgen_update_func( update_mapgen_id( extra.generator_id ), dat ).success();
            break;
        }
        case map_extra_method::null:
        default:
            break;
    }

    if( !applied_successfully ) {
        return;
    }

    overmap_buffer.add_extra( abs_omt, id );
}

FunctionMap all_functions()
{
    return builtin_functions;
}

void load( const JsonObject &jo, const std::string &src )
{
    extras.load( jo, src );
}

void check_consistency()
{
    extras.check();
}

void debug_spawn_test()
{
    uilist mx_menu;
    std::vector<std::string> mx_names;
    const region_settings_map_extras &settings_map_extras =
        region_settings_default->get_settings_map_extras();
    for( const map_extra_collection_id &region_extra : settings_map_extras.extras ) {
        const std::string &mx_name = region_extra.str();
        mx_menu.addentry( -1, true, -1, mx_name );
        mx_names.push_back( mx_name );
    }

    mx_menu.text = _( "Test which map extra list?" );
    while( true ) {
        mx_menu.query();
        const int index = mx_menu.ret;
        if( index >= static_cast<int>( mx_names.size() ) || index < 0 ) {
            break;
        }

        std::map<map_extra_id, int> results;
        map_extra_id mx_null = map_extra_id::NULL_ID();

        for( size_t a = 0; a < 32400; a++ ) {
            auto mx_iter = settings_map_extras.extras.find( map_extra_collection_id( mx_names[index] ) );
            if( mx_iter == settings_map_extras.extras.end() ) {
                continue;
            }
            const map_extra_collection &ex = **mx_iter;
            if( ex.chance > 0 && one_in( ex.chance ) ) {
                const map_extra_id *extra = ex.values.pick();
                if( extra == nullptr ) {
                    results[mx_null]++;
                } else {
                    results[*ex.values.pick()]++;
                }
            } else {
                results[mx_null]++;
            }
        }

        std::multimap<int, map_extra_id> sorted_results;
        for( std::pair<const map_extra_id, int> &e : results ) {
            sorted_results.emplace( e.second, e.first );
        }
        uilist results_menu;
        results_menu.text = _( "Result of 32400 selections:" );
        for( std::pair<const int, map_extra_id> &r : sorted_results ) {
            results_menu.entries.emplace_back(
                static_cast<int>( results_menu.entries.size() ), true, -2,
                string_format( "%d x %s", r.first, r.second.str() ) );
        }
        results_menu.query();
    }
}

} // namespace MapExtras

// gross
void map_extra::finalize_all()
{
    extras.finalize();
}

bool map_extra::is_valid_for( const mapgendata &md ) const
{
    int z = md.zlevel();
    if( min_max_zlevel_ ) {
        if( z < min_max_zlevel_->first || z > min_max_zlevel_->second ) {
            return false;
        }
    }
    if( !md.region.overmap_feature_flag.blacklist.empty() ) {
        // map extra is blacklisted by flag
        if( cata::sets_intersect( md.region.overmap_feature_flag.blacklist, get_flags() ) ) {
            return false;
        }
    }
    if( !md.region.overmap_feature_flag.whitelist.empty() ) {
        // map extra is not whitelisted by flag
        if( !cata::sets_intersect( md.region.overmap_feature_flag.whitelist, get_flags() ) ) {
            return false;
        }
    }

    return true;
}

void map_extra::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "description", description_ );
    if( jo.has_object( "generator" ) ) {
        JsonObject jg = jo.get_object( "generator" );
        generator_method = jg.get_enum_value<map_extra_method>( "generator_method",
                           map_extra_method::null );
        mandatory( jg, was_loaded, "generator_id", generator_id );
    }
    optional( jo, was_loaded, "sym", symbol, unicode_codepoint_from_symbol_reader, NULL_UNICODE );
    color = jo.has_member( "color" ) ? color_from_string( jo.get_string( "color" ) ) : was_loaded ?
            color : c_white;
    optional( jo, was_loaded, "autonote", autonote, false );
    optional( jo, was_loaded, "min_max_zlevel", min_max_zlevel_ );
    optional( jo, was_loaded, "flags", flags_, string_reader{} );
    if( was_loaded && jo.has_member( "extend" ) ) {
        JsonObject joe = jo.get_object( "extend" );
        for( auto &flag : joe.get_string_array( "flags" ) ) {
            flags_.insert( flag );
        }
    }
    if( was_loaded && jo.has_member( "delete" ) ) {
        JsonObject joe = jo.get_object( "extend" );
        for( auto &flag : joe.get_string_array( "flags" ) ) {
            flags_.erase( flag );
        }
    }
}

void map_extra::finalize() const
{
    if( generator_method != map_extra_method::null ) {
        MapExtras::all_function_names.push_back( id );
    }
}

void map_extra::check() const
{
    switch( generator_method ) {
        case map_extra_method::map_extra_function: {
            const map_extra_pointer mx_func = MapExtras::get_function( map_extra_id( generator_id ) );
            if( mx_func == nullptr ) {
                debugmsg( "invalid map extra function (%s) defined for map extra (%s)", generator_id, id.str() );
            }
            break;
        }
        case map_extra_method::update_mapgen: {
            const auto update_mapgen_func = update_mapgens.find( update_mapgen_id( generator_id ) );
            if( update_mapgen_func == update_mapgens.end() ||
                update_mapgen_func->second.funcs().empty() ) {
                debugmsg( "invalid update mapgen function (%s) defined for map extra (%s)", generator_id,
                          id.str() );
            }
            break;
        }
        case map_extra_method::mapgen:
        case map_extra_method::null:
        default:
            break;
    }
}
