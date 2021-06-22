#include "map_helpers.h"

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "cata_assert.h"
#include "character.h"
#include "clzones.h"
#include "faction.h"
#include "field.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "item_pocket.h"
#include "location.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "npc.h"
#include "point.h"
#include "ret_val.h"
#include "submap.h"
#include "type_id.h"

// Remove all vehicles from the map
void clear_vehicles()
{
    map &here = get_map();
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
                here.set_radiation( { x, y, z}, 0 );
            }
        }
    }
}

void wipe_map_terrain()
{
    map &here = get_map();
    const int mapsize = here.getmapsize() * SEEX;
    for( int z = -1; z <= OVERMAP_HEIGHT; ++z ) {
        ter_id terrain = z == 0 ? t_grass : z < 0 ? t_rock : t_open_air;
        for( int x = 0; x < mapsize; ++x ) {
            for( int y = 0; y < mapsize; ++y ) {
                here.set( { x, y, z}, terrain, f_null );
            }
        }
    }
    clear_vehicles();
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
    // Reload to ensure that all active NPCs are in the overmap_buffer.
    g->reload_npcs();
    for( npc &n : g->all_npcs() ) {
        n.die( nullptr );
    }
    g->cleanup_dead();
}

void clear_fields( const int zlevel )
{
    map &here = get_map();
    const int mapsize = here.getmapsize() * SEEX;
    for( int x = 0; x < mapsize; ++x ) {
        for( int y = 0; y < mapsize; ++y ) {
            const tripoint p( x, y, zlevel );
            point offset;

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
            here.i_clear( { x, y, zlevel } );
        }
    }
}

void clear_zones()
{
    zone_manager &zm = zone_manager::get_manager();
    for( auto zone_ref : zm.get_zones( faction_id( "your_followers" ) ) ) {
        if( !zone_ref.get().get_is_vehicle() ) {
            // Trying to delete vehicle zones fails with a message that the zone isn't loaded.
            // Don't need it right now and the errors spam up the test output, so skip.
            continue;
        }
        zm.remove( zone_ref.get() );
    }
}

void clear_map()
{
    // Clearing all z-levels is rather slow, so just clear the ones I know the
    // tests use for now.
    for( int z = -2; z <= 0; ++z ) {
        clear_fields( z );
    }
    clear_zones();
    wipe_map_terrain();
    clear_npcs();
    clear_creatures();
    get_map().clear_traps();
    for( int z = -2; z <= 0; ++z ) {
        clear_items( z );
    }
}

void clear_map_and_put_player_underground()
{
    clear_map();
    // Make sure the player doesn't block the path of the monster being tested.
    get_player_location().setpos( { 0, 0, -2 } );
}

monster &spawn_test_monster( const std::string &monster_type, const tripoint &start )
{
    monster *const added = g->place_critter_at( mtype_id( monster_type ), start );
    cata_assert( added );
    return *added;
}

// Build a map of size MAPSIZE_X x MAPSIZE_Y around tripoint_zero with a given
// terrain, and no furniture, traps, or items.
void build_test_map( const ter_id &terrain )
{
    map &here = get_map();
    for( const tripoint &p : here.points_in_rectangle( tripoint_zero,
            tripoint( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        here.furn_set( p, furn_id( "f_null" ) );
        here.ter_set( p, terrain );
        here.trap_set( p, trap_id( "tr_null" ) );
        here.i_clear( p );
    }

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
}

void player_add_headlamp()
{
    item headlamp( "wearable_light_on" );
    item battery( "light_battery_cell" );
    battery.ammo_set( battery.ammo_default(), -1 );
    headlamp.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );
    get_player_character().worn.push_back( headlamp );
}
