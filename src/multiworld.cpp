
#include <climits>
#include <cstdint>
#include <iosfwd>
#include <new>
#include <optional>
#include <string>
#include <vector>

#include <multiworld.h>
#include <map.h>
#include <avatar.h>
#include <game.h>
#include <weather.h>
#include <mapbuffer.h>
#include <overmapbuffer.h>

multiworld::multiworld() = default;
multiworld::~multiworld() = default;
multiworld MULTIWORLD;

void multiworld::set_world_prefix( std::string prefix )
{
    world_prefix = std::move( prefix );
}
std::string multiworld::get_world_prefix()
{
    return world_prefix;
}
bool multiworld::travel_to_world( const std::string &prefix )
{
    map &here = get_map();
    avatar &player = get_avatar();
    //unload NPCs
    g->unload_npcs();
    //unload monsters
    for( monster &critter : g->all_monsters() ) {
        g->despawn_monster( critter );
    }
    if( player.in_vehicle ) {
        here.unboard_vehicle( player.pos_bub() );
    }
    //make sure we don't mess up savedata if for some reason maps can't be saved
    if( !g->save_maps() ) {
        return false;
    }
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        here.clear_vehicle_list( z );
    }
    here.rebuild_vehicle_level_caches();
    /*inputting an empty string to the text input EOC fails
    so i'm using 'default' as empty/main world */
    if( prefix != "default" ) {
        set_world_prefix( prefix );
    } else {
        set_world_prefix( "" );
    }
    MAPBUFFER.clear();
    //FIXME hack to prevent crashes from temperature checks
    //this returns to false in 'on_turn()' so it should be fine?
    g->swapping_worlds = true;
    //in theory if we skipped the next two lines we'd have an exact copy of the overmap from the past world, only with differences noticeable in the local map.
    overmap_buffer.clear();
    //load/create new overmap
    overmap_buffer.get( point_abs_om{} );
    //loads submaps
    here.load( tripoint_abs_sm( here.get_abs_sub() ), false );
    here.access_cache( here.get_abs_sub().z() ).map_memory_cache_dec.reset();
    here.access_cache( here.get_abs_sub().z() ).map_memory_cache_ter.reset();
    here.invalidate_visibility_cache();
    g->load_npcs();
    here.spawn_monsters( true, true ); // Static monsters
    g->update_overmap_seen();
    // update weather now as it could be different on the new location
    get_weather().nextweather = calendar::turn;
    return true;
}
