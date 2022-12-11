#include "map_helpers.h"

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_assert.h"
#include "character.h"
#include "clzones.h"
#include "faction.h"
#include "field.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "item_pocket.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "npc.h"
#include "point.h"
#include "ret_val.h"
#include "submap.h"
#include "type_id.h"

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
                here.set_radiation( { x, y, z}, 0 );
            }
        }
    }
}

void wipe_map_terrain( map *target )
{
    map &here = target ? *target : get_map();
    const int mapsize = here.getmapsize() * SEEX;
    for( int z = -1; z <= OVERMAP_HEIGHT; ++z ) {
        ter_id terrain = z == 0 ? t_grass : z < 0 ? t_rock : t_open_air;
        for( int x = 0; x < mapsize; ++x ) {
            for( int y = 0; y < mapsize; ++y ) {
                here.set( { x, y, z}, terrain, f_null );
                here.partial_con_remove( { x, y, z } );
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
    zm.clear();
}

void clear_map()
{
    map &here = get_map();
    // Clearing all z-levels is rather slow, so just clear the ones I know the
    // tests use for now.
    for( int z = -2; z <= 0; ++z ) {
        clear_fields( z );
    }
    clear_zones();
    wipe_map_terrain();
    clear_npcs();
    clear_creatures();
    here.clear_traps();
    for( int z = -2; z <= 0; ++z ) {
        clear_items( z );
    }
}

void clear_map_and_put_player_underground()
{
    clear_map();
    // Make sure the player doesn't block the path of the monster being tested.
    get_player_character().setpos( { 0, 0, -2 } );
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
    Character &you = get_player_character();
    you.worn.wear_item( you, headlamp, false, true );
}

void player_wear_blindfold()
{
    item blindfold( "blindfold" );
    Character &you = get_player_character();
    you.worn.wear_item( you, blindfold, false, true );
}

void set_time_to_day()
{
    time_point noon = calendar::turn - time_past_midnight( calendar::turn ) + 12_hours;
    if( noon < calendar::turn ) {
        noon = noon + 1_days;
    }
    set_time( noon );
}

// Set current time of day, and refresh map and caches for the new light level
void set_time( const time_point &time )
{
    calendar::turn = time;
    g->reset_light_level();
    Character &you = get_player_character();
    int z = you.posz();
    you.recalc_sight_limits();
    map &here = get_map();
    here.update_visibility_cache( z );
    here.invalidate_map_cache( z );
    here.build_map_cache( z );
}
