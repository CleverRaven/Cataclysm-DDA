#include "map_helpers.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "creature_tracker.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "npc.h"
#include "field.h"
#include "game_constants.h"
#include "pimpl.h"
#include "type_id.h"
#include "point.h"

void wipe_map_terrain()
{
    const int mapsize = g->m.getmapsize() * SEEX;
    for( int z = 0; z <= OVERMAP_HEIGHT; ++z ) {
        ter_id terrain = z == 0 ? t_grass : t_open_air;
        for( int x = 0; x < mapsize; ++x ) {
            for( int y = 0; y < mapsize; ++y ) {
                g->m.set( { x, y, z}, terrain, f_null );
            }
        }
    }
    for( wrapped_vehicle &veh : g->m.get_vehicles() ) {
        g->m.destroy_vehicle( veh.v );
    }
    g->m.invalidate_map_cache( 0 );
    g->m.build_map_cache( 0, true );
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
    const int mapsize = g->m.getmapsize() * SEEX;
    for( int x = 0; x < mapsize; ++x ) {
        for( int y = 0; y < mapsize; ++y ) {
            const tripoint p( x, y, zlevel );
            std::vector<field_type_id> fields;
            for( auto &pr : g->m.field_at( p ) ) {
                fields.push_back( pr.second.get_field_type() );
            }
            for( field_type_id f : fields ) {
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
    clear_npcs();
    clear_creatures();
    g->m.clear_traps();
}

void clear_map_and_put_player_underground()
{
    clear_map();
    // Make sure the player doesn't block the path of the monster being tested.
    g->u.setpos( { 0, 0, -2 } );
}

monster &spawn_test_monster( const std::string &monster_type, const tripoint &start )
{
    std::shared_ptr<monster> temp = std::make_shared<monster>( mtype_id( monster_type ), start );
    // Bypassing game::add_zombie() since it sometimes upgrades the monster instantly.
    const bool was_added = g->critter_tracker->add( temp );
    assert( was_added );
    return *temp;
}

