#include "creature_tracker.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "player.h"
#include "field.h"
#include "overmapbuffer.h"
#include "mapgen_functions.h"
#include "map_iterator.h"
#include "omdata.h"

void wipe_map_terrain()
{
    // Remove all the obstacles.
    const int mapsize = g->m.getmapsize() * SEEX;
    for( int x = 0; x < mapsize; ++x ) {
        for( int y = 0; y < mapsize; ++y ) {
            g->m.set( x, y, t_grass, f_null );
        }
    }
    for( wrapped_vehicle &veh : g->m.get_vehicles() ) {
        g->m.destroy_vehicle( veh.v );
    }
    g->m.build_map_cache( 0, true );
}

// removes all items in a 1 radius area around a point
void i_clear_adjacent( const tripoint &p )
{
    auto close_trip = g->m.points_in_radius( p, 1 );
    for( const auto &trip : close_trip ) {
        g->m.i_clear( trip );
    }
}

// generates a thick forest overmap tile. sets the adjacent tiles to thick forests as well.
void generate_forest_OMT( const tripoint &p )
{
    oter_id f( "forest_thick" );
    wipe_map_terrain();

    // first, set current and adjacent map tiles to forest_thick
    for( int x = -1; x <= 1; x++ ) {
        for( int y = -1; y <= 1; y++ ) {
            const tripoint temp_trip( p.x + x, p.y + y, p.z );
            overmap_buffer.ter( temp_trip ) = f;
        }
    }
    const regional_settings &rsettings = overmap_buffer.get_settings( p.x, p.y, p.z );
    mapgendata mgd( f, f, f, f, f, f, f, f, f, f, 0, rsettings, g->m );
    mapgen_forest( &g->m, f, mgd, calendar::turn, 0 );
}

void clear_creatures()
{
    // Remove any interfering monsters.
    g->clear_zombies();
    g->unload_npcs();
}

void clear_fields( const int zlevel )
{
    const int mapsize = g->m.getmapsize() * SEEX;
    for( int x = 0; x < mapsize; ++x ) {
        for( int y = 0; y < mapsize; ++y ) {
            const tripoint p( x, y, zlevel );
            std::vector<field_id> fields;
            for( auto &pr : g->m.field_at( p ) ) {
                fields.push_back( pr.second.getFieldType() );
            }
            for( field_id f : fields ) {
                g->m.remove_field( p, f );
            }
        }
    }
}

void clear_map()
{
    // Clearing all z-levels is rather slow, so just clear the ones I know the
    // tests use for now.
    for( int z = -2; z <= 0; ++z ) {
        clear_fields( z );
    }
    wipe_map_terrain();
    clear_creatures();
}

void clear_map_and_put_player_underground()
{
    clear_map();
    // Make sure the player doesn't block the path of the monster being tested.
    g->u.setpos( { 0, 0, -2 } );
}

monster &spawn_test_monster( const std::string &monster_type, const tripoint &start )
{
    monster temp_monster( mtype_id( monster_type ), start );
    // Bypassing game::add_zombie() since it sometimes upgrades the monster instantly.
    g->critter_tracker->add( temp_monster );
    return *g->critter_tracker->find( temp_monster.pos() );
}

