#include "map_helpers_tests.h"

#include <memory>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "monster.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "submap.h"
#include "type_id.h"

static const itype_id itype_blindfold( "blindfold" );
static const itype_id itype_medium_battery_cell( "medium_battery_cell" );
static const itype_id itype_wearable_light_on( "wearable_light_on" );

static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_rock( "t_rock" );

monster &spawn_test_monster( const std::string &monster_type, const tripoint_bub_ms &start,
                             const bool death_drops )
{
    mtype_id type( monster_type );
    REQUIRE( !type.is_null() );
    REQUIRE( get_creature_tracker().creature_at( start ) == nullptr );
    monster mon( type );
    map &here = get_map();
    CAPTURE( here.ter( start ) );
    CAPTURE( here.furn( start ) );
    CAPTURE( here.tr_at( start ) );
    CAPTURE( here.move_cost( start ) );
    REQUIRE( mon.will_move_to( start ) );
    REQUIRE( mon.know_danger_at( start ) );

    monster *const test_monster_ptr = g->place_critter_at( type, start );
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

    clear_map_with_vision( z_bottom - 1, z_surface + 1, /* with_vision = */ false );

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

bool map_meddler::has_altered_submaps( map &m )
{
    for( submap *sm : m.grid ) {
        if( sm->player_adjusted_map ) {
            return true;
        }
    }
    return false;
}

submap *map_meddler::unsafe_get_submap_at( tripoint_bub_ms &p, point_sm_ms &l )
{
    return get_map().unsafe_get_submap_at( p, l );
}
