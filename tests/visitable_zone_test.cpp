#include "cata_catch.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "monster.h"
#include "rng.h"

#include <vector>

// The area of the whole map from two below to one above ground level.
static const tripoint p1( 0, 0, -2 );
static const tripoint p2( ( MAPSIZE *SEEX ) - 1, ( MAPSIZE *SEEY ) - 1, 1 );

struct structure {
    structure( const tripoint &origin, const tripoint &dimensions, const std::string &name ) :
        area( origin, origin + dimensions ), name_( name ) {};
    bool contains( const tripoint &p ) const {
        return area.contains( p );
    }
    inclusive_cuboid<tripoint> area;
    std::string name_;
};

static void place_structures( const std::vector<structure> &spawn_areas,
                              std::vector<std::vector<tripoint>> &interior_areas,
                              std::vector<tripoint> &outside_area,
                              std::vector<tripoint> &rooftop_area )
{
    map &here = get_map();
    for( const tripoint &p : here.points_in_rectangle( p1, p2 ) ) {
        bool found = false;
        for( unsigned int i = 0; i < spawn_areas.size(); ++i ) {
            // Indexing so these can pair up with interior_areas.
            const structure &spawn_area = spawn_areas[i];
            if( spawn_area.contains( p ) ) {
                found = true;
                // Uppermost level (if any) gets a roof covering the whole footprint.
                if( spawn_area.area.p_max.z != spawn_area.area.p_min.z && p.z == spawn_area.area.p_max.z ) {
                    rooftop_area.emplace_back( p );
                    here.ter_set( p, ter_id( "t_flat_roof" ) );
                    break;
                }
                inclusive_cuboid<tripoint> interior{
                    rectangle{
                        spawn_area.area.p_min.xy() + point_south_east,
                        spawn_area.area.p_max.xy() - point_south_east
                    },
                    spawn_area.area.p_min.z, spawn_area.area.p_min.z
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
        if( !found && p.z == 0 ) {
            outside_area.push_back( p );
        }
    }
}

// Just make all the buildings the same size.
static constexpr int building_width = 7;
static constexpr int building_height = 1;
static constexpr tripoint building_offset{ building_width, building_width, building_height };
static constexpr tripoint cave_offset{ building_width, building_width, 0 };

TEST_CASE( "visitable_zone_surface_test" )
{
    map &here = get_map();
    clear_map();

    std::string mon_type = "mon_zombie";
    std::vector<monster *> monsters;
    // All of these origins must be building_width + 3 away from each other.
    // Surface building locations.
    const tripoint enclosed_building{ 30, 30, 0 };
    const tripoint closed_door = enclosed_building + tripoint_east * 2;
    // Surface building with a door.
    const tripoint door_building{ 40, 30, 0 };
    const tripoint open_door = door_building + tripoint_east * 2;
    // Surface building with a window.
    const tripoint window_building{ 50, 30, 0 };
    const tripoint window = window_building + tripoint_south * 2;
    // Underground room.
    const tripoint underground_room{ 60, 30, -1 };
    // Underground room.
    const tripoint underground_stairs_room{ 70, 30, -1 };
    const tripoint underground_stairs_up = underground_stairs_room + tripoint_south_east * 3;
    const tripoint underground_stairs_down = underground_stairs_up + tripoint_above;
    // Elevated building with stairs to the surface.
    const tripoint stilts_building{ 80, 30, 1 };
    const tripoint stilts_stairs_down = stilts_building + tripoint_south_east * 3;
    const tripoint stilts_stairs_up = stilts_stairs_down + tripoint_below;

    std::vector<structure> spawn_locations = {{
            { enclosed_building, building_offset, "closed door building" },
            { door_building, building_offset, "open door building" },
            { window_building, building_offset, "window building" },
            { underground_room, cave_offset, "cave" },
            { underground_stairs_room, cave_offset, "cave with stairs" },
            { stilts_building, building_offset, "building on stilts" }
        }
    };
    std::vector<std::vector<tripoint>> interior_areas( spawn_locations.size() );
    std::vector<tripoint> outside_area;
    std::vector<tripoint> rooftop_area;
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
    REQUIRE( !here.passable( enclosed_building + point_east ) );
    REQUIRE( !here.passable( door_building + point_south ) );
    REQUIRE( !here.is_transparent_wo_fields( enclosed_building + point_east ) );
    REQUIRE( !here.is_transparent_wo_fields( door_building + point_south ) );

    REQUIRE( here.passable( open_door ) );
    REQUIRE( here.is_transparent_wo_fields( open_door ) );

    REQUIRE( !here.passable( closed_door ) );
    REQUIRE( !here.is_transparent_wo_fields( closed_door ) );

    REQUIRE( !here.passable( window ) );
    REQUIRE( here.is_transparent_wo_fields( window ) );

    for( std::vector<tripoint> &area : interior_areas ) {
        for( const int &choice : rng_sequence( 2, 0, area.size() - 1 ) ) {
            monster &mon = spawn_test_monster( mon_type, area[choice] );
            monsters.push_back( &mon );
        }
    }
    for( const int &choice : rng_sequence( 5, 0, outside_area.size() - 1 ) ) {
        monster &mon = spawn_test_monster( mon_type, outside_area[choice] );
        monsters.push_back( &mon );
    }
    for( const int &choice : rng_sequence( 5, 0, rooftop_area.size() - 1 ) ) {
        monster &mon = spawn_test_monster( mon_type, rooftop_area[choice] );
        monsters.push_back( &mon );
    }
    std::set<int> zones;
    for( monster *mon : monsters ) {
        mon->plan();
        zones.insert( mon->get_reachable_zone() );
    }
    for( monster *mon : monsters ) {
        tripoint mpos = mon->pos();
        std::vector<structure>::iterator it =
            find_if( spawn_locations.begin(), spawn_locations.end(),
        [mon]( structure & a_structure ) {
            return a_structure.contains( mon->pos() );
        } );
        std::string spawn_site = "outside";
        if( mpos.z >= 1 ) {
            spawn_site = "rooftop";
        } else if( it != spawn_locations.end() ) {
            spawn_site = it->name_;
        }
        CAPTURE( spawn_site );
        CAPTURE( mon->pos() );
        CAPTURE( mon->get_reachable_zone() );
        CHECK( mon->get_reachable_zone() != 0 );
        CHECK( zones.size() == 3 );
    }
}
