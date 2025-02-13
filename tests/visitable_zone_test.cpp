#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "cata_catch.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "monster.h"
#include "point.h"
#include "rng.h"
#include "type_id.h"

// The area of the whole map from two below to one above ground level.
static const tripoint_bub_ms p1( 0, 0, -2 );
static const tripoint_bub_ms p2( ( MAPSIZE *SEEX ) - 1, ( MAPSIZE *SEEY ) - 1, 1 );

struct structure {
    structure( const tripoint_bub_ms &origin, const tripoint_rel_ms &dimensions,
               const std::string &name ) :
        area( origin, origin + dimensions ), name_( name ) {};
    bool contains( const tripoint_bub_ms &p ) const {
        return area.contains( p );
    }
    inclusive_cuboid<tripoint_bub_ms> area;
    std::string name_;
};

static void place_structures( const std::vector<structure> &spawn_areas,
                              std::vector<std::vector<tripoint_bub_ms>> &interior_areas,
                              std::vector<tripoint_bub_ms> &outside_area,
                              std::vector<tripoint_bub_ms> &rooftop_area )
{
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_rectangle( p1, p2 ) ) {
        bool found = false;
        for( unsigned int i = 0; i < spawn_areas.size(); ++i ) {
            // Indexing so these can pair up with interior_areas.
            const structure &spawn_area = spawn_areas[i];
            if( spawn_area.contains( p ) ) {
                found = true;
                // Uppermost level (if any) gets a roof covering the whole footprint.
                if( spawn_area.area.p_max.z() != spawn_area.area.p_min.z() && p.z() == spawn_area.area.p_max.z() ) {
                    rooftop_area.emplace_back( p );
                    here.ter_set( p, ter_id( "t_flat_roof" ) );
                    break;
                }
                inclusive_cuboid<tripoint_bub_ms> interior{
                    rectangle<point_bub_ms>{
                        spawn_area.area.p_min.xy() + point::south_east,
                        spawn_area.area.p_max.xy() - point::south_east
                    },
                    spawn_area.area.p_min.z(), spawn_area.area.p_min.z()
                };
                if( interior.contains( p ) ) {
                    here.ter_set( p, ter_id( "t_floor" ) );
                    interior_areas[i].emplace_back( p );
                } else {
                    here.ter_set( p, ter_id( "t_concrete_wall" ) );
                }
                break;
            }
        }
        if( !found && p.z() == 0 ) {
            outside_area.push_back( p );
        }
    }
}

// Just make all the buildings the same size.
static constexpr int building_width = 7;
static constexpr int building_height = 1;
static constexpr tripoint_rel_ms building_offset{ building_width, building_width, building_height };
static constexpr tripoint_rel_ms cave_offset{ building_width, building_width, 0 };

TEST_CASE( "visitable_zone_surface_test" )
{
    map &here = get_map();
    clear_map();

    std::string mon_type = "mon_zombie";
    std::vector<monster *> monsters;
    // All of these origins must be building_width + 3 away from each other.
    // Surface building locations.
    const tripoint_bub_ms enclosed_building{ 30, 30, 0 };
    const tripoint_bub_ms closed_door = enclosed_building + tripoint::east * 2;
    // Surface building with a door.
    const tripoint_bub_ms door_building{ 40, 30, 0 };
    const tripoint_bub_ms open_door = door_building + tripoint::east * 2;
    // Surface building with a window.
    const tripoint_bub_ms window_building{ 50, 30, 0 };
    const tripoint_bub_ms window = window_building + tripoint::south * 2;
    // Underground room.
    const tripoint_bub_ms underground_room{ 60, 30, -1 };
    // Underground room.
    const tripoint_bub_ms underground_stairs_room{ 70, 30, -1 };
    const tripoint_bub_ms underground_stairs_up = underground_stairs_room +
            tripoint::south_east * 3;
    const tripoint_bub_ms underground_stairs_down = underground_stairs_up + tripoint::above;
    // Elevated building with stairs to the surface.
    const tripoint_bub_ms stilts_building{ 80, 30, 1 };
    const tripoint_bub_ms stilts_stairs_down = stilts_building + tripoint::south_east * 3;
    const tripoint_bub_ms stilts_stairs_up = stilts_stairs_down + tripoint::below;

    std::vector<structure> spawn_locations = {{
            { enclosed_building, building_offset, "closed door building" },
            { door_building, building_offset, "open door building" },
            { window_building, building_offset, "window building" },
            { underground_room, cave_offset, "cave" },
            { underground_stairs_room, cave_offset, "cave with stairs" },
            { stilts_building, building_offset, "building on stilts" }
        }
    };
    std::vector<std::vector<tripoint_bub_ms>> interior_areas( spawn_locations.size() );
    std::vector<tripoint_bub_ms> outside_area;
    std::vector<tripoint_bub_ms> rooftop_area;
    place_structures( spawn_locations, interior_areas, outside_area, rooftop_area );
    here.ter_set( closed_door, ter_id( "t_door_c" ) );
    here.ter_set( open_door, ter_id( "t_door_o" ) );
    here.ter_set( window, ter_id( "t_window" ) );
    here.ter_set( underground_stairs_up, ter_id( "t_wood_stairs_up" ) );
    here.ter_set( underground_stairs_down, ter_id( "t_wood_stairs_down" ) );
    here.ter_set( stilts_stairs_up, ter_id( "t_wood_stairs_up" ) );
    here.ter_set( stilts_stairs_down, ter_id( "t_wood_stairs_down" ) );

    // Reset all the map caches after editing the map.
    for( int i = OVERMAP_HEIGHT; i >= -OVERMAP_DEPTH; --i ) {
        here.invalidate_map_cache( i );
        here.build_map_cache( i, true );
    }

    // Some spot checks of our map edits.
    REQUIRE( !here.passable( enclosed_building + point::east ) );
    REQUIRE( !here.passable( door_building + point::south ) );
    REQUIRE( !here.is_transparent_wo_fields( tripoint_bub_ms( enclosed_building + point::east ) ) );
    REQUIRE( !here.is_transparent_wo_fields( tripoint_bub_ms( door_building + point::south ) ) );

    REQUIRE( here.passable( open_door ) );
    REQUIRE( here.is_transparent_wo_fields( tripoint_bub_ms( open_door ) ) );

    REQUIRE( !here.passable( closed_door ) );
    REQUIRE( !here.is_transparent_wo_fields( tripoint_bub_ms( closed_door ) ) );

    REQUIRE( !here.passable( window ) );
    REQUIRE( here.is_transparent_wo_fields( tripoint_bub_ms( window ) ) );

    for( std::vector<tripoint_bub_ms> &area : interior_areas ) {
        for( const int &choice : rng_sequence( 2, 0, area.size() - 1 ) ) {
            monster &mon = spawn_test_monster( mon_type, tripoint_bub_ms( area[choice] ) );
            monsters.push_back( &mon );
        }
    }
    for( const int &choice : rng_sequence( 5, 0, outside_area.size() - 1 ) ) {
        monster &mon = spawn_test_monster( mon_type, tripoint_bub_ms( outside_area[choice] ) );
        monsters.push_back( &mon );
    }
    for( const int &choice : rng_sequence( 5, 0, rooftop_area.size() - 1 ) ) {
        monster &mon = spawn_test_monster( mon_type, tripoint_bub_ms( rooftop_area[choice] ) );
        monsters.push_back( &mon );
    }
    std::unordered_map<int, int> count_by_zone;
    for( monster *mon : monsters ) {
        mon->plan();
        const int zone = mon->get_reachable_zone();
        ++count_by_zone[zone];
    }
    CHECK( count_by_zone.size() == 3 );

    for( monster *mon : monsters ) {
        tripoint_bub_ms mpos = mon->pos_bub();
        std::vector<structure>::iterator it =
            find_if( spawn_locations.begin(), spawn_locations.end(),
        [mon]( structure & a_structure ) {
            return a_structure.contains( mon->pos_bub() );
        } );
        std::string spawn_site = "outside";
        if( mpos.z() >= 1 ) {
            spawn_site = "rooftop";
        } else if( it != spawn_locations.end() ) {
            spawn_site = it->name_;
        }
        CAPTURE( spawn_site );
        CAPTURE( mon->pos_bub() );
        CAPTURE( mon->get_reachable_zone() );
        CHECK( mon->get_reachable_zone() != 0 );
    }

    creature_tracker &tracker = get_creature_tracker();
    for( monster *mon : monsters ) {
        const int zone = mon->get_reachable_zone();
        int visited_count = 0;
        tracker.for_each_reachable( *mon, [zone, &visited_count]( Creature * creature ) {
            CHECK( zone == creature->get_reachable_zone() );
            // Don't count the player.
            if( creature->is_monster() ) {
                ++visited_count;
            }
        } );

        CHECK( visited_count == count_by_zone[zone] );
    }
}
