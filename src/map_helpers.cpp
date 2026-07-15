#include "map_helpers.h"

#include <bitset>
#include <memory>
#include <unordered_map>
#include <vector>

#include "avatar.h"
#include "character.h"
#include "clzones.h"
#include "coordinates.h"
#include "field.h"
#include "game.h"
#include "mapbuffer.h"
#include "map.h"
#include "map_scale_constants.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "point.h"
#include "submap.h"
#include "type_id.h"
#include "weather.h"

static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_rock( "t_rock" );

// Remove all vehicles from the map
void clear_vehicles( map *target )
{
    map &here = target ? *target : get_map();
    for( wrapped_vehicle &veh : here.get_vehicles() ) {
        here.destroy_vehicle( veh.v );
    }
}

void clear_radiation()
{
    map &here = get_map();
    const int mapsize = here.getmapsize() * SEEX;
    for( int z = -1; z <= OVERMAP_HEIGHT; ++z ) {
        for( int x = 0; x < mapsize; ++x ) {
            for( int y = 0; y < mapsize; ++y ) {
                here.set_radiation( tripoint_bub_ms{ x, y, z }, 0 );
            }
        }
    }
}

void wipe_map_terrain( map *target )
{
    wipe_map_terrain_with_vision( target, true );
}

void wipe_map_terrain_with_vision( map *target, bool with_vision )
{
    map &here = target ? *target : get_map();
    const int mapsize = here.getmapsize() * SEEX;
    for( int z = -1; z <= OVERMAP_HEIGHT; ++z ) {
        ter_id terrain = z == 0 ? ter_t_grass : z < 0 ? ter_t_rock : ter_t_open_air;
        for( int x = 0; x < mapsize; ++x ) {
            for( int y = 0; y < mapsize; ++y ) {
                here.set( tripoint_bub_ms{ x, y, z}, terrain, furn_str_id::NULL_ID() );
                if( with_vision ) {
                    here.partial_con_remove( { x, y, z } );
                } else {
                    here.partial_con_remove_no_vision_for_testing( { x, y, z } );
                }
            }
        }
    }
    clear_vehicles( target );
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
}

void clear_creatures()
{
    // Remove any interfering monsters.
    g->clear_zombies();
}

void clear_npcs()
{
    map &here = get_map();
    // Reload to ensure that all active NPCs are in the overmap_buffer.
    g->reload_npcs();
    for( npc &n : g->all_npcs() ) {
        n.die( & here, nullptr );
    }
    g->cleanup_dead();
}

void clear_fields( const int zlevel )
{
    map &here = get_map();
    const int mapsize = here.getmapsize() * SEEX;
    for( int x = 0; x < mapsize; ++x ) {
        for( int y = 0; y < mapsize; ++y ) {
            const tripoint_bub_ms p( x, y, zlevel );
            point_sm_ms offset;

            submap *sm = here.get_submap_at( p, offset );
            if( sm ) {
                sm->field_count = 0;
                sm->get_field( offset ).clear();
            }
        }
    }
}

void clear_items( const int zlevel )
{
    map &here = get_map();
    const int mapsize = here.getmapsize() * SEEX;
    for( int x = 0; x < mapsize; ++x ) {
        for( int y = 0; y < mapsize; ++y ) {
            here.i_clear( tripoint_bub_ms{ x, y, zlevel } );
        }
    }
}

void clear_zones()
{
    zone_manager &zm = zone_manager::get_manager();
    zm.clear();
}

void clear_basecamps()
{
    overmap_buffer.clear_camps( get_avatar().pos_abs_omt().xy() );
}

static std::bitset<24 * 24> impassable_omt;
static std::bitset<24 * 24> passable_omt{ ~impassable_omt };

void clear_map( int zmin, int zmax )
{
    clear_map_with_vision( zmin, zmax, true );
}

void clear_map_without_vision( int zmin, int zmax )
{
    clear_map_with_vision( zmin, zmax, false );
}

void clear_map_with_vision( int zmin, int zmax, bool with_vision )
{
    map &here = get_map();
    if( const tripoint_abs_sm &abs_sub = here.get_abs_sub(); abs_sub.z() != 0 ) {
        // Reset z level to 0
        here.load( tripoint_abs_sm( abs_sub.xy(), 0 ), false );
    }
    // Clearing all z-levels is rather slow, so just clear the ones I know the
    // tests use for now.
    for( int z = zmin; z <= zmax; ++z ) {
        clear_fields( z );
    }
    clear_zones();
    clear_npcs();
    wipe_map_terrain_with_vision( nullptr, with_vision );
    clear_creatures();
    here.clear_traps();
    for( int z = zmin; z <= zmax; ++z ) {
        clear_items( z );
    }
    here.process_items();
    clear_basecamps();
    get_weather().snow_depth_map.clear();
    // Set a chunk of overmap passability cache to all passable.
    const tripoint_abs_sm &abs_sub = here.get_abs_sub();
    const tripoint_abs_omt abs_omt = project_to<coords::omt>( abs_sub );
    overmap_buffer.clear_mongroups();
    for( int y = -4; y < 10; ++y ) {
        for( int x = -4; x < 10; ++x ) {
            tripoint_abs_omt this_omt{ abs_omt.x() + x, abs_omt.y() + y, 0 };
            overmap_buffer.set_passable( this_omt, passable_omt );
        }
    }
}

void clear_map_and_put_player_underground()
{
    map &here = get_map();
    clear_map_without_vision();
    // Make sure the player doesn't block the path of the monster being tested.
    get_player_character().setpos( here, tripoint_bub_ms{ 0, 0, -2 } );
}

void clear_overmaps()
{
    // Just drop all generated overmaps.
    overmap_buffer.clear();
    // Also all generated submaps.
    MAPBUFFER.clear();
    // Just make a new map.
    get_map() = map();
    g->place_player_overmap( tripoint_abs_omt() );
}
