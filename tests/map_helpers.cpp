#include "map_helpers.h"

#include <memory>
#include <optional>
#include <vector>

#include "avatar.h"
#include "basecamp.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "clzones.h"
#include "coordinates.h"
#include "field.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "monster.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "submap.h"
#include "type_id.h"

static const itype_id itype_blindfold( "blindfold" );
static const itype_id itype_medium_battery_cell( "medium_battery_cell" );
static const itype_id itype_wearable_light_on( "wearable_light_on" );

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
                here.set_radiation( tripoint_bub_ms{ x, y, z}, 0 );
            }
        }
    }
}

void wipe_map_terrain( map *target )
{
    map &here = target ? *target : get_map();
    const int mapsize = here.getmapsize() * SEEX;
    for( int z = -1; z <= OVERMAP_HEIGHT; ++z ) {
        ter_id terrain = z == 0 ? ter_t_grass : z < 0 ? ter_t_rock : ter_t_open_air;
        for( int x = 0; x < mapsize; ++x ) {
            for( int y = 0; y < mapsize; ++y ) {
                here.set( tripoint_bub_ms{ x, y, z}, terrain, furn_str_id::NULL_ID() );
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
    std::optional<basecamp *> camp;
    do {
        const tripoint_abs_omt &avatar_pos = get_avatar().pos_abs_omt();
        camp = overmap_buffer.find_camp( avatar_pos.xy() );
        if( camp && *camp != nullptr ) {
            ( **camp ).remove_camp( avatar_pos );
        }
    } while( camp );
}

void clear_map( int zmin, int zmax )
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
    wipe_map_terrain();
    clear_creatures();
    here.clear_traps();
    for( int z = zmin; z <= zmax; ++z ) {
        clear_items( z );
    }
    here.process_items();
    clear_basecamps();
}

void clear_map_and_put_player_underground()
{
    map &here = get_map();
    clear_map();
    // Make sure the player doesn't block the path of the monster being tested.
    get_player_character().setpos( here, tripoint_bub_ms{ 0, 0, -2 } );
}

monster &spawn_test_monster( const std::string &monster_type, const tripoint_bub_ms &start,
                             const bool death_drops )
{
    monster *const test_monster_ptr = g->place_critter_at( mtype_id( monster_type ), start );
    REQUIRE( test_monster_ptr );
    test_monster_ptr->death_drops = death_drops;
    return *test_monster_ptr;
}

// Build a map of size MAPSIZE_X x MAPSIZE_Y around tripoint::zero with a given
// terrain, and no furniture, traps, or items.
void build_test_map( const ter_id &terrain )
{
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_rectangle( tripoint_bub_ms::zero,
            tripoint_bub_ms( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        here.furn_set( p, furn_id( "f_null" ) );
        here.ter_set( p, terrain );
        here.trap_set( p, trap_id( "tr_null" ) );
        here.i_clear( p );
    }

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
}

void build_water_test_map( const ter_id &surface, const ter_id &mid, const ter_id &bottom )
{
    constexpr int z_surface = 0;
    constexpr int z_bottom = -2;

    clear_map( z_bottom - 1, z_surface + 1 );

    map &here = get_map();
    const tripoint_bub_ms p1( 0, 0, z_bottom - 1 );
    const tripoint_bub_ms p2( MAPSIZE * SEEX, MAPSIZE * SEEY, z_surface + 1 );
    for( const tripoint_bub_ms &p : here.points_in_rectangle( p1, p2 ) ) {

        if( p.z() == z_surface ) {
            here.ter_set( p, surface );
        } else if( p.z() < z_surface && p.z() > z_bottom ) {
            here.ter_set( p, mid );
        } else if( p.z() == z_bottom ) {
            here.ter_set( p, bottom );
        } else if( p.z() < z_bottom ) {
            here.ter_set( p, ter_t_rock );
        } else if( p.z() > z_surface ) {
            here.ter_set( p, ter_t_open_air );
        }
    }

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
}

void player_add_headlamp()
{
    item headlamp( itype_wearable_light_on );
    item battery( itype_medium_battery_cell );
    battery.ammo_set( battery.ammo_default(), -1 );
    headlamp.put_in( battery, pocket_type::MAGAZINE_WELL );
    Character &you = get_player_character();
    you.worn.wear_item( you, headlamp, false, true );
}

void player_wear_blindfold()
{
    item blindfold( itype_blindfold );
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
    here.invalidate_visibility_cache();
    here.update_visibility_cache( z );
    here.invalidate_map_cache( z );
    here.build_map_cache( z );
}
