#include "creature_tracker.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "player.h"
#include "field.h"

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
    g->m.clear_traps();
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

